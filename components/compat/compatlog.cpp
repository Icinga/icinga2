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
#include "icinga/downtimemessage.h"
#include "icinga/service.h"
#include "icinga/checkcommand.h"
#include "icinga/notification.h"
#include "icinga/notificationmessage.h"
#include "icinga/macroprocessor.h"
#include "config/configcompilercontext.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/exception.h"
#include "base/convert.h"
#include "base/application.h"
#include "base/utility.h"
#include "base/scriptfunction.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(CompatLog);
REGISTER_SCRIPTFUNCTION(ValidateRotationMethod, &CompatLog::ValidateRotationMethod);

CompatLog::CompatLog(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate), m_LastRotation(0)
{
	RegisterAttribute("log_dir", Attribute_Config, &m_LogDir);
	RegisterAttribute("rotation_method", Attribute_Config, &m_RotationMethod);
}

/**
 * @threadsafety Always.
 */
void CompatLog::OnAttributeChanged(const String& name)
{
	ASSERT(!OwnsLock());

	if (name == "rotation_method")
		ScheduleNextRotation();
}

/**
 * @threadsafety Always.
 */
void CompatLog::Start(void)
{
	m_Endpoint = Endpoint::MakeEndpoint("compatlog_" + GetName(), false);
	m_Endpoint->RegisterTopicHandler("checker::CheckResult",
	    boost::bind(&CompatLog::CheckResultRequestHandler, this, _3));
	m_Endpoint->RegisterTopicHandler("icinga::NotificationSent",
	    boost::bind(&CompatLog::NotificationSentRequestHandler, this, _3));

	Service::OnDowntimeChanged.connect(bind(&CompatLog::DowntimeHandler, this, _1, _2));

	m_RotationTimer = boost::make_shared<Timer>();
	m_RotationTimer->OnTimerExpired.connect(boost::bind(&CompatLog::RotationTimerHandler, this));
	m_RotationTimer->Start();

	ReopenFile(false);
	ScheduleNextRotation();
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
		return Application::GetLocalStateDir() + "/log/icinga2/compat";
}

/**
 * @threadsafety Always.
 */
String CompatLog::GetRotationMethod(void) const
{
	if (!m_RotationMethod.IsEmpty())
		return m_RotationMethod;
	else
		return "HOURLY";
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

        String raw_output;
        String output;

        if (cr) {
                raw_output = cr->Get("output");
                size_t line_end = raw_output.Find("\n");

                output = raw_output.SubStr(0, line_end);

                boost::algorithm::replace_all(output, "\n", "\\n");
        }

	std::ostringstream msgbuf;
	msgbuf << "SERVICE ALERT: "
	       << host->GetName() << ";"
	       << service->GetShortName() << ";"
	       << Service::StateToString(static_cast<ServiceState>(state_after)) << ";"
	       << Service::StateTypeToString(static_cast<StateType>(stateType_after)) << ";"
	       << attempt_after << ";"
	       << output << ""
	       << "";

	{
		ObjectLock olock(this);
		WriteLine(msgbuf.str());
	}

	if (service == host->GetHostCheckService()) {
		std::ostringstream msgbuf;
		msgbuf << "HOST ALERT: "
		       << host->GetName() << ";"
		       << Host::StateToString(Host::CalculateState(static_cast<ServiceState>(state_after), host_reachable_after)) << ";"
		       << Service::StateTypeToString(static_cast<StateType>(stateType_after)) << ";"
		       << attempt_after << ";"
		       << output << ""
		       << "";

		{
			ObjectLock olock(this);
			WriteLine(msgbuf.str());
		}

	}

	{
		ObjectLock olock(this);
		Flush();
	}
}

/**
 * @threadsafety Always.
 */
void CompatLog::DowntimeHandler(const Service::Ptr& service, DowntimeState downtime_state)
{
	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	String downtime_state_str;
	String downtime_output;

	switch (downtime_state) {
		case DowntimeStarted:
			downtime_output = "Service has entered a period of scheduled downtime.";
			downtime_state_str = "STARTED";
			break;
		case DowntimeStopped:
			downtime_output = "Service has exited from a period of scheduled downtime.";
			downtime_state_str = "STOPPED";
			break;
		case DowntimeCancelled:
			downtime_output = "Scheduled downtime for service has been cancelled.";
			downtime_state_str = "CANCELLED";
			break;
		default:
			Log(LogCritical, "compat", "Unknown downtime state: " + Convert::ToString(downtime_state));
			return;
	}

	std::ostringstream msgbuf;
	msgbuf << "SERVICE DOWNTIME ALERT: "
		<< host->GetName() << ";"
		<< service->GetShortName() << ";"
		<< downtime_state_str << "; "
		<< downtime_output
		<< "";

	{
		ObjectLock oLock(this);
		WriteLine(msgbuf.str());
	}

	if (service == host->GetHostCheckService()) {
		std::ostringstream msgbuf;
		msgbuf << "HOST DOWNTIME ALERT: "
			<< host->GetName() << ";"
			<< downtime_state_str << "; "
			<< downtime_output
			<< "";

		{
			ObjectLock oLock(this);
			WriteLine(msgbuf.str());
		}
	}

	{
		ObjectLock oLock(this);
		Flush();
	}
}

/**
 * @threadsafety Always.
 */
void CompatLog::NotificationSentRequestHandler(const RequestMessage& request)
{
        NotificationMessage params;
        if (!request.GetParams(&params))
                return;

        String svcname = params.GetService();
        Service::Ptr service = Service::GetByName(svcname);

        Host::Ptr host = service->GetHost();

        if (!host)
                return;

	String username = params.GetUser();
	String author = params.GetAuthor();
	String comment_text = params.GetCommentText();

	CheckCommand::Ptr commandObj = service->GetCheckCommand();

	String check_command = "";
	if (commandObj)
		check_command = commandObj->GetName();

	NotificationType notification_type = params.GetType();
	String notification_type_str = Notification::NotificationTypeToString(notification_type);

	String author_comment = "";
	if (notification_type == NotificationCustom || notification_type == NotificationAcknowledgement) {
		author_comment = ";" + author + ";" + comment_text;
	}

        Dictionary::Ptr cr = params.GetCheckResult();
        if (!cr)
                return;

	String output;
	String raw_output;

        if (cr) {
                raw_output = cr->Get("output");
                size_t line_end = raw_output.Find("\n");

                output = raw_output.SubStr(0, line_end);
        }

        std::ostringstream msgbuf;
        msgbuf << "SERVICE NOTIFICATION: "
		<< username << ";"
                << host->GetName() << ";"
                << service->GetShortName() << ";"
                << notification_type_str << " "
		<< "(" << Service::StateToString(service->GetState()) << ");"
		<< check_command << ";"
		<< raw_output << author_comment
                << "";

        {
                ObjectLock oLock(this);
                WriteLine(msgbuf.str());
        }

        if (service == host->GetHostCheckService()) {
                std::ostringstream msgbuf;
                msgbuf << "HOST NOTIFICATION: "
			<< username << ";"
                        << host->GetName() << ";"
                	<< notification_type_str << " "
			<< "(" << Service::StateToString(service->GetState()) << ");"
			<< check_command << ";"
			<< raw_output << author_comment
                        << "";

                {
                        ObjectLock oLock(this);
                        WriteLine(msgbuf.str());
                }
        }

        {
                ObjectLock oLock(this);
                Flush();
        }
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
void CompatLog::ReopenFile(bool rotate)
{
	ObjectLock olock(this);

	String tempFile = GetLogDir() + "/icinga.log";

	if (m_OutputFile) {
		m_OutputFile.close();

		if (rotate) {
			String archiveFile = GetLogDir() + "/archives/icinga-" + Utility::FormatDateTime("%m-%d-%Y-%H", Utility::GetTime()) + ".log";

			Log(LogInformation, "compat", "Rotating compat log file '" + tempFile + "' -> '" + archiveFile + "'");
			(void) rename(tempFile.CStr(), archiveFile.CStr());
		}
	}

	m_OutputFile.open(tempFile.CStr(), std::ofstream::app);

	if (!m_OutputFile.good()) {
		Log(LogWarning, "icinga", "Could not open compat log file '" + tempFile + "' for writing. Log output will be lost.");

		return;
	}

	WriteLine("LOG ROTATION: " + GetRotationMethod());
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

void CompatLog::ScheduleNextRotation(void)
{
	time_t now = (time_t)Utility::GetTime();
	String method = GetRotationMethod();

	tm tmthen;

#ifdef _MSC_VER
	tm *temp = localtime(&now);

	if (temp == NULL) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("localtime")
		    << boost::errinfo_errno(errno));
	}

	tmthen = *temp;
#else /* _MSC_VER */
	if (localtime_r(&now, &tmthen) == NULL) {
		BOOST_THROW_EXCEPTION(posix_error()
		    << boost::errinfo_api_function("localtime_r")
		    << boost::errinfo_errno(errno));
	}
#endif /* _MSC_VER */

	tmthen.tm_min = 0;
	tmthen.tm_sec = 0;

	if (method == "HOURLY") {
		tmthen.tm_hour++;
	} else if (method == "DAILY") {
		tmthen.tm_mday++;
		tmthen.tm_hour = 0;
	} else if (method == "WEEKLY") {
		tmthen.tm_mday += 7 - tmthen.tm_wday;
		tmthen.tm_hour = 0;
	} else if (method == "MONTHLY") {
		tmthen.tm_mon++;
		tmthen.tm_mday = 1;
		tmthen.tm_hour = 0;
	}

	time_t ts = mktime(&tmthen);

	Log(LogInformation, "compat", "Rescheduling rotation timer for compat log '"
	    + GetName() + "' to '" + Utility::FormatDateTime("%Y/%m/%d %H:%M:%S %z", ts) + "'");
	m_RotationTimer->Reschedule(ts);
}

/**
 * @threadsafety Always.
 */
void CompatLog::RotationTimerHandler(void)
{
	try {
		ReopenFile(true);
	} catch (...) {
		ScheduleNextRotation();

		throw;
	}

	ScheduleNextRotation();
}

void CompatLog::ValidateRotationMethod(const String& location, const Dictionary::Ptr& attrs)
{
	Value rotation_method = attrs->Get("rotation_method");

	if (!rotation_method.IsEmpty() && rotation_method != "HOURLY" && rotation_method != "DAILY" &&
	    rotation_method != "WEEKLY" && rotation_method != "MONTHLY" && rotation_method != "NONE") {
		ConfigCompilerContext::GetContext()->AddError(false, "Validation failed for " +
		    location + ": Rotation method '" + rotation_method + "' is invalid.");
	}
}
