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

#include "icinga/perfdatawriter.h"
#include "icinga/checkresultmessage.h"
#include "icinga/service.h"
#include "icinga/macroprocessor.h"
#include "icinga/icingaapplication.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/application.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

REGISTER_TYPE(PerfdataWriter);

PerfdataWriter::PerfdataWriter(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	RegisterAttribute("perfdata_path", Attribute_Config, &m_PerfdataPath);
	RegisterAttribute("format_template", Attribute_Config, &m_FormatTemplate);
	RegisterAttribute("rotation_interval", Attribute_Config, &m_RotationInterval);
}

void PerfdataWriter::OnAttributeChanged(const String& name)
{
	ASSERT(!OwnsLock());

	if (name == "rotation_interval") {
		m_RotationTimer->SetInterval(GetRotationInterval());
	}
}

void PerfdataWriter::Start(void)
{
	m_Endpoint = Endpoint::MakeEndpoint("perfdata_" + GetName(), false);
	m_Endpoint->RegisterTopicHandler("checker::CheckResult",
	    boost::bind(&PerfdataWriter::CheckResultRequestHandler, this, _3));

	m_RotationTimer = boost::make_shared<Timer>();
	m_RotationTimer->OnTimerExpired.connect(boost::bind(&PerfdataWriter::RotationTimerHandler, this));
	m_RotationTimer->SetInterval(GetRotationInterval());
	m_RotationTimer->Start();

	RotateFile();
}

PerfdataWriter::Ptr PerfdataWriter::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("PerfdataWriter", name);

	return dynamic_pointer_cast<PerfdataWriter>(configObject);
}

String PerfdataWriter::GetPerfdataPath(void) const
{
	if (!m_PerfdataPath.IsEmpty())
		return m_PerfdataPath;
	else
		return Application::GetLocalStateDir() + "/cache/icinga2/perfdata/perfdata";
}

String PerfdataWriter::GetFormatTemplate(void) const
{
	if (!m_FormatTemplate.IsEmpty()) {
		return m_FormatTemplate;
	} else {
		return "DATATYPE::SERVICEPERFDATA\t"
			"TIMET::$TIMET$\t"
			"HOSTNAME::$HOSTNAME$\t"
			"SERVICEDESC::$SERVICEDESC$\t"
			"SERVICEPERFDATA::$SERVICEPERFDATA$\t"
			"SERVICECHECKCOMMAND::$SERVICECHECKCOMMAND$\t"
			"HOSTSTATE::$HOSTSTATE$\t"
			"HOSTSTATETYPE::$HOSTSTATETYPE$\t"
			"SERVICESTATE::$SERVICESTATE$\t"
			"SERVICESTATETYPE::$SERVICESTATETYPE$";
	}
}

double PerfdataWriter::GetRotationInterval(void) const
{
	if (!m_RotationInterval.IsEmpty())
		return m_RotationInterval;
	else
		return 30;
}

void PerfdataWriter::CheckResultRequestHandler(const RequestMessage& request)
{
	CheckResultMessage params;
	if (!request.GetParams(&params))
		return;

	String svcname = params.GetService();
	Service::Ptr service = Service::GetByName(svcname);

	Dictionary::Ptr cr = params.GetCheckResult();
	if (!cr)
		return;

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(service);
	resolvers.push_back(service->GetHost());
	resolvers.push_back(IcingaApplication::GetInstance());

	String line = MacroProcessor::ResolveMacros(GetFormatTemplate(), resolvers, cr);

	ObjectLock olock(this);
	if (!m_OutputFile.good())
		return;

	m_OutputFile << line << "\n";
}

void PerfdataWriter::RotateFile(void)
{
	ObjectLock olock(this);

	String tempFile = GetPerfdataPath();

	if (m_OutputFile.good()) {
		m_OutputFile.close();

		String finalFile = GetPerfdataPath() + "." + Convert::ToString((long)Utility::GetTime());
		(void) rename(tempFile.CStr(), finalFile.CStr());
	}

	m_OutputFile.open(tempFile.CStr());

	if (!m_OutputFile.good())
		Log(LogWarning, "icinga", "Could not open perfdata file '" + tempFile + "' for writing. Perfdata will be lost.");
}

void PerfdataWriter::RotationTimerHandler(void)
{
	RotateFile();
}
