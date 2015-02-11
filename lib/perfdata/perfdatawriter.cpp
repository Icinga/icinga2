/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "perfdata/perfdatawriter.hpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/dynamictype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/context.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"
#include "base/statsfunction.hpp"

using namespace icinga;

REGISTER_TYPE(PerfdataWriter);
REGISTER_SCRIPTFUNCTION(ValidateFormatTemplates, &PerfdataWriter::ValidateFormatTemplates);

REGISTER_STATSFUNCTION(PerfdataWriterStats, &PerfdataWriter::StatsFunc);

void PerfdataWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	BOOST_FOREACH(const PerfdataWriter::Ptr& perfdatawriter, DynamicType::GetObjectsByType<PerfdataWriter>()) {
		nodes->Set(perfdatawriter->GetName(), 1); //add more stats
	}

	status->Set("perfdatawriter", nodes);
}

void PerfdataWriter::Start(void)
{
	DynamicObject::Start();

	Checkable::OnNewCheckResult.connect(boost::bind(&PerfdataWriter::CheckResultHandler, this, _1, _2));

	m_RotationTimer = new Timer();
	m_RotationTimer->OnTimerExpired.connect(boost::bind(&PerfdataWriter::RotationTimerHandler, this));
	m_RotationTimer->SetInterval(GetRotationInterval());
	m_RotationTimer->Start();

	RotateFile(m_ServiceOutputFile, GetServiceTempPath(), GetServicePerfdataPath());
	RotateFile(m_HostOutputFile, GetHostTempPath(), GetHostPerfdataPath());
}

Value PerfdataWriter::EscapeMacroMetric(const Value& value)
{
	if (value.IsObjectType<Array>())
		return Utility::Join(value, ';');
	else
		return value;
}

void PerfdataWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	CONTEXT("Writing performance data for object '" + checkable->GetName() + "'");

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	Service::Ptr service = dynamic_pointer_cast<Service>(checkable);
	Host::Ptr host;

	if (service)
		host = service->GetHost();
	else
		host = static_pointer_cast<Host>(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.push_back(std::make_pair("service", service));
	resolvers.push_back(std::make_pair("host", host));
	resolvers.push_back(std::make_pair("icinga", IcingaApplication::GetInstance()));

	if (service) {
		String line = MacroProcessor::ResolveMacros(GetServiceFormatTemplate(), resolvers, cr, NULL, &PerfdataWriter::EscapeMacroMetric);

		{
			ObjectLock olock(this);
			if (!m_ServiceOutputFile.good())
				return;

			m_ServiceOutputFile << line << "\n";
		}
	} else {
		String line = MacroProcessor::ResolveMacros(GetHostFormatTemplate(), resolvers, cr, NULL, &PerfdataWriter::EscapeMacroMetric);

		{
			ObjectLock olock(this);
			if (!m_HostOutputFile.good())
				return;

			m_HostOutputFile << line << "\n";
		}
	}
}

void PerfdataWriter::RotateFile(std::ofstream& output, const String& temp_path, const String& perfdata_path)
{
	ObjectLock olock(this);

	if (output.good()) {
		output.close();

		String finalFile = perfdata_path + "." + Convert::ToString((long)Utility::GetTime());
		(void) rename(temp_path.CStr(), finalFile.CStr());
	}

	output.open(temp_path.CStr());

	if (!output.good())
		Log(LogWarning, "PerfdataWriter")
		    << "Could not open perfdata file '" << temp_path << "' for writing. Perfdata will be lost.";
}

void PerfdataWriter::RotationTimerHandler(void)
{
	RotateFile(m_ServiceOutputFile, GetServiceTempPath(), GetServicePerfdataPath());
	RotateFile(m_HostOutputFile, GetHostTempPath(), GetHostPerfdataPath());
}

void PerfdataWriter::ValidateFormatTemplates(const String& location, const PerfdataWriter::Ptr& object)
{
	if (!Utility::ValidateMacroString(object->GetHostFormatTemplate())) {
		BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
		    location + ": Closing $ not found in macro format string '" + object->GetHostFormatTemplate() + "'.", object->GetDebugInfo()));
	}

	if (!Utility::ValidateMacroString(object->GetServiceFormatTemplate())) {
		BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
		    location + ": Closing $ not found in macro format string '" + object->GetHostFormatTemplate() + "'.", object->GetDebugInfo()));
	}
}
