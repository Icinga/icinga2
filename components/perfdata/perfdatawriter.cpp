/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "perfdata/perfdatawriter.h"
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

PerfdataWriter::PerfdataWriter(void)
	: m_RotationInterval(30)
{ }

void PerfdataWriter::Start(void)
{
	DynamicObject::Start();

	Service::OnNewCheckResult.connect(boost::bind(&PerfdataWriter::CheckResultHandler, this, _1, _2));

	m_RotationTimer = boost::make_shared<Timer>();
	m_RotationTimer->OnTimerExpired.connect(boost::bind(&PerfdataWriter::RotationTimerHandler, this));
	m_RotationTimer->SetInterval(GetRotationInterval());
	m_RotationTimer->Start();

	RotateFile();
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
	return m_RotationInterval;
}

void PerfdataWriter::CheckResultHandler(const Service::Ptr& service, const Dictionary::Ptr& cr)
{
	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !service->GetEnablePerfdata())
		return;

	Host::Ptr host = service->GetHost();

	if (!host)
		return;

	std::vector<MacroResolver::Ptr> resolvers;
	resolvers.push_back(service);
	resolvers.push_back(host);
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

void PerfdataWriter::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("perfdata_path", m_PerfdataPath);
		bag->Set("format_template", m_FormatTemplate);
		bag->Set("rotation_interval", m_RotationInterval);
	}
}

void PerfdataWriter::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_PerfdataPath = bag->Get("perfdata_path");
		m_FormatTemplate = bag->Get("format_template");
		m_RotationInterval = bag->Get("rotation_interval");
	}
}
