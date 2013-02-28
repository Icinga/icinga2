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

#include "i2-icinga.h"

using namespace icinga;

REGISTER_TYPE(PerfdataWriter, NULL);

PerfdataWriter::PerfdataWriter(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	RegisterAttribute("path_prefix", Attribute_Config, &m_PathPrefix);
	RegisterAttribute("format_template", Attribute_Config, &m_FormatTemplate);
	RegisterAttribute("rotation_interval", Attribute_Config, &m_RotationInterval);
}

PerfdataWriter::~PerfdataWriter(void)
{
}

void PerfdataWriter::OnAttributeChanged(const String& name, const Value& oldValue)
{
	if (name == "rotation_interval") {
		ObjectLock olock(this);
		m_RotationTimer->SetInterval(GetRotationInterval());
	}
}

void PerfdataWriter::Start(void)
{
	m_Endpoint = Endpoint::MakeEndpoint("perfdata_" + GetName(), false);

	{
		ObjectLock olock(m_Endpoint);
		m_Endpoint->RegisterTopicHandler("checker::CheckResult",
		    boost::bind(&PerfdataWriter::CheckResultRequestHandler, this, _3));
	}

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

String PerfdataWriter::GetPathPrefix(void) const
{
	if (!m_PathPrefix.IsEmpty())
		return m_PathPrefix;
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

	Dictionary::Ptr macros = cr->Get("macros");
	String line = MacroProcessor::ResolveMacros(GetFormatTemplate(), macros);

	if (!m_OutputFile.good())
		return;

	ObjectLock olock(this);
	m_OutputFile << line << "\n";
}

void PerfdataWriter::RotateFile(void)
{
	String tempFile = GetPathPrefix();

	if (m_OutputFile.good()) {
		m_OutputFile.close();

		String finalFile = GetPathPrefix() + "." + Convert::ToString((long)Utility::GetTime());
		(void) rename(tempFile.CStr(), finalFile.CStr());
	}

	m_OutputFile.open(tempFile.CStr());

	if (!m_OutputFile.good())
		Logger::Write(LogWarning, "icinga", "Could not open perfdata file '" + tempFile + "' for writing. Perfdata will be lost.");
}

void PerfdataWriter::RotationTimerHandler(void)
{
	ObjectLock olock(this);
	RotateFile();
}
