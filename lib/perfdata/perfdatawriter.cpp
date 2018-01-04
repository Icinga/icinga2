/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "perfdata/perfdatawriter.tcpp"
#include "icinga/service.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/configtype.hpp"
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

REGISTER_STATSFUNCTION(PerfdataWriter, &PerfdataWriter::StatsFunc);

void PerfdataWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const PerfdataWriter::Ptr& perfdatawriter : ConfigType::GetObjectsByType<PerfdataWriter>()) {
		nodes->Set(perfdatawriter->GetName(), 1); //add more stats
	}

	status->Set("perfdatawriter", nodes);
}

void PerfdataWriter::Start(bool runtimeCreated)
{
	ObjectImpl<PerfdataWriter>::Start(runtimeCreated);

	Log(LogInformation, "PerfdataWriter")
		<< "'" << GetName() << "' started.";

	Checkable::OnNewCheckResult.connect(std::bind(&PerfdataWriter::CheckResultHandler, this, _1, _2));

	m_RotationTimer = new Timer();
	m_RotationTimer->OnTimerExpired.connect(std::bind(&PerfdataWriter::RotationTimerHandler, this));
	m_RotationTimer->SetInterval(GetRotationInterval());
	m_RotationTimer->Start();

	RotateFile(m_ServiceOutputFile, GetServiceTempPath(), GetServicePerfdataPath());
	RotateFile(m_HostOutputFile, GetHostTempPath(), GetHostPerfdataPath());
}

void PerfdataWriter::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "PerfdataWriter")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<PerfdataWriter>::Stop(runtimeRemoved);
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
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	if (service) {
		String line = MacroProcessor::ResolveMacros(GetServiceFormatTemplate(), resolvers, cr, nullptr, &PerfdataWriter::EscapeMacroMetric);

		{
			ObjectLock olock(this);
			if (!m_ServiceOutputFile.good())
				return;

			m_ServiceOutputFile << line << "\n";
		}
	} else {
		String line = MacroProcessor::ResolveMacros(GetHostFormatTemplate(), resolvers, cr, nullptr, &PerfdataWriter::EscapeMacroMetric);

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

		if (Utility::PathExists(temp_path)) {
			String finalFile = perfdata_path + "." + Convert::ToString((long)Utility::GetTime());
			if (rename(temp_path.CStr(), finalFile.CStr()) < 0) {
				BOOST_THROW_EXCEPTION(posix_error()
					<< boost::errinfo_api_function("rename")
					<< boost::errinfo_errno(errno)
					<< boost::errinfo_file_name(temp_path));
			}
		}
	}

	output.open(temp_path.CStr());

	if (!output.good())
		Log(LogWarning, "PerfdataWriter")
			<< "Could not open perfdata file '" << temp_path << "' for writing. Perfdata will be lost.";
}

void PerfdataWriter::RotationTimerHandler()
{
	RotateFile(m_ServiceOutputFile, GetServiceTempPath(), GetServicePerfdataPath());
	RotateFile(m_HostOutputFile, GetHostTempPath(), GetHostPerfdataPath());
}

void PerfdataWriter::ValidateHostFormatTemplate(const String& value, const ValidationUtils& utils)
{
	ObjectImpl<PerfdataWriter>::ValidateHostFormatTemplate(value, utils);

	if (!MacroProcessor::ValidateMacroString(value))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "host_format_template" }, "Closing $ not found in macro format string '" + value + "'."));
}

void PerfdataWriter::ValidateServiceFormatTemplate(const String& value, const ValidationUtils& utils)
{
	ObjectImpl<PerfdataWriter>::ValidateServiceFormatTemplate(value, utils);

	if (!MacroProcessor::ValidateMacroString(value))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "service_format_template" }, "Closing $ not found in macro format string '" + value + "'."));
}
