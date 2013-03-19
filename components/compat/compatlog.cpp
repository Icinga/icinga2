/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "compat/compatlog.h"
#include "icinga/checkresultmessage.h"
#include "icinga/service.h"
#include "icinga/macroprocessor.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include "base/application.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(CompatLog);

CompatLog::CompatLog(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	RegisterAttribute("log_dir", Attribute_Config, &m_LogDir);
	RegisterAttribute("rotation_interval", Attribute_Config, &m_RotationInterval);
}

CompatLog::~CompatLog(void)
{
}

/**
 * @threadsafety Always.
 */
void CompatLog::OnAttributeChanged(const String& name)
{
	ASSERT(!OwnsLock());

	if (name == "rotation_interval") {
		m_RotationTimer->SetInterval(GetRotationInterval());
	}
}

/**
 * @threadsafety Always.
 */
void CompatLog::Start(void)
{
	m_Endpoint = Endpoint::MakeEndpoint("compatlog_" + GetName(), false);
	m_Endpoint->RegisterTopicHandler("checker::CheckResult",
	    boost::bind(&CompatLog::CheckResultRequestHandler, this, _3));

	m_RotationTimer = boost::make_shared<Timer>();
	m_RotationTimer->OnTimerExpired.connect(boost::bind(&CompatLog::RotationTimerHandler, this));
	m_RotationTimer->SetInterval(GetRotationInterval());
	m_RotationTimer->Start();

	RotateFile();
}

/**
 * @threadsafety Always.
 */
CompatLog::Ptr CompatLog::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("CompatLog", name);

	return dynamic_pointer_cast<CompatLog>(configObject);
}

/**
 * @threadsafety Always.
 */
String CompatLog::GetLogDir(void) const
{
	if (!m_LogDir.IsEmpty())
		return m_LogDir;
	else
		return Application::GetLocalStateDir() + "/log/icinga2/compat/";
}

/**
 * @threadsafety Always.
 */
double CompatLog::GetRotationInterval(void) const
{
	if (!m_RotationInterval.IsEmpty())
		return m_RotationInterval;
	else
		return 3600;
}

/**
 * @threadsafety Always.
 */
void CompatLog::CheckResultRequestHandler(const RequestMessage& request)
{
	CheckResultMessage params;
	if (!request.GetParams(&params))
		return;

	String svcname = params.GetService();
	Service::Ptr service = Service::GetByName(svcname);

	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	Dictionary::Ptr cr = params.GetCheckResult();
	if (!cr)
		return;

	Dictionary::Ptr vars_after = cr->Get("vars_after");

	long state_after = vars_after->Get("state");
	long stateType_after = vars_after->Get("state_type");
	long attempt_after = vars_after->Get("attempt");
	bool reachable_after = vars_after->Get("reachable");
	bool host_reachable_after = vars_after->Get("host_reachable");

	Dictionary::Ptr vars_before = cr->Get("vars_before");

	if (vars_before) {
		long state_before = vars_before->Get("state");
		long stateType_before = vars_before->Get("state_type");
		long attempt_before = vars_before->Get("attempt");
		bool reachable_before = vars_before->Get("reachable");

		if (state_before == state_after && stateType_before == stateType_after &&
		    attempt_before == attempt_after && reachable_before == reachable_after)
			return; /* Nothing changed, ignore this checkresult. */
	}

	std::ostringstream msgbuf;
	msgbuf << "SERVICE ALERT: "
	       << host->GetName() << ";"
	       << service->GetShortName() << ";"
	       << Service::StateToString(static_cast<ServiceState>(state_after)) << ";"
	       << Service::StateTypeToString(static_cast<StateType>(stateType_after)) << ";"
	       << attempt_after << ";"
	       << "";

	WriteLine(msgbuf.str());

	if (service == host->GetHostCheckService()) {
		std::ostringstream msgbuf;
		msgbuf << "HOST ALERT: "
		       << host->GetName() << ";"
		       << Host::StateToString(Host::CalculateState(static_cast<ServiceState>(state_after), host_reachable_after)) << ";"
		       << Service::StateTypeToString(static_cast<StateType>(stateType_after)) << ";"
		       << attempt_after << ";"
		       << "";

		WriteLine(msgbuf.str());
	}

	Flush();
}

void CompatLog::WriteLine(const String& line)
{
	ASSERT(OwnsLock());

	if (!m_OutputFile.good())
		return;

	m_OutputFile << "[" << (long)Utility::GetTime() << "] " << line << "\n";
}

void CompatLog::Flush(void)
{
	ASSERT(OwnsLock());

	if (!m_OutputFile.good())
		return;

	m_OutputFile << std::flush;
}

/**
 * @threadsafety Always.
 */
void CompatLog::RotateFile(void)
{
	ObjectLock olock(this);

	String tempFile = GetLogDir() + "/icinga.log";

	if (m_OutputFile.good()) {
		m_OutputFile.close();

		String finalFile = GetLogDir() + "/archives/icinga-" + Convert::ToString((long)Utility::GetTime()) + ".log";
		(void) rename(tempFile.CStr(), finalFile.CStr());
	}

	m_OutputFile.open(tempFile.CStr());

	if (!m_OutputFile.good()) {
		Log(LogWarning, "icinga", "Could not open compat log file '" + tempFile + "' for writing. Log output will be lost.");

		return;
	}

	WriteLine("LOG VERSION: 2.0");

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Host")) {
		Host::Ptr host = static_pointer_cast<Host>(object);

		Service::Ptr hc = host->GetHostCheckService();

		if (!hc)
			continue;

		bool reachable = host->IsReachable();

		ObjectLock olock(hc);

		std::ostringstream msgbuf;
		msgbuf << "HOST STATE: CURRENT;"
		       << host->GetName() << ";"
		       << Host::StateToString(Host::CalculateState(hc->GetState(), reachable)) << ";"
		       << Service::StateTypeToString(hc->GetStateType()) << ";"
		       << hc->GetCurrentCheckAttempt() << ";"
		       << "";

		WriteLine(msgbuf.str());
	}

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = static_pointer_cast<Service>(object);

		Host::Ptr host = service->GetHost();

		if (!host)
			continue;

		std::ostringstream msgbuf;
		msgbuf << "SERVICE STATE: CURRENT;"
		       << host->GetName() << ";"
		       << service->GetShortName() << ";"
		       << Service::StateToString(service->GetState()) << ";"
		       << Service::StateTypeToString(service->GetStateType()) << ";"
		       << service->GetCurrentCheckAttempt() << ";"
		       << "";

		WriteLine(msgbuf.str());
	}

	Flush();
}

/**
 * @threadsafety Always.
 */
void CompatLog::RotationTimerHandler(void)
{
	RotateFile();
}
