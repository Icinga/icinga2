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

#include "icinga/compatutility.hpp"
#include "compat/checkresultreader.hpp"
#include "compat/checkresultreader.tcpp"
#include "icinga/service.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/application.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/context.hpp"
#include "base/statsfunction.hpp"
#include <fstream>

using namespace icinga;

REGISTER_TYPE(CheckResultReader);

REGISTER_STATSFUNCTION(CheckResultReader, &CheckResultReader::StatsFunc);

void CheckResultReader::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const CheckResultReader::Ptr& checkresultreader : ConfigType::GetObjectsByType<CheckResultReader>()) {
		nodes->Set(checkresultreader->GetName(), 1); //add more stats
	}

	status->Set("checkresultreader", nodes);
}

/**
 * @threadsafety Always.
 */
void CheckResultReader::Start(bool runtimeCreated)
{
	ObjectImpl<CheckResultReader>::Start(runtimeCreated);

	Log(LogInformation, "CheckResultReader")
		<< "'" << GetName() << "' started.";

#ifndef _WIN32
	m_ReadTimer = new Timer();
	m_ReadTimer->OnTimerExpired.connect(std::bind(&CheckResultReader::ReadTimerHandler, this));
	m_ReadTimer->SetInterval(5);
	m_ReadTimer->Start();
#endif /* _WIN32 */
}

/**
 * @threadsafety Always.
 */
void CheckResultReader::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "CheckResultReader")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<CheckResultReader>::Stop(runtimeRemoved);
}

/**
 * @threadsafety Always.
 */
void CheckResultReader::ReadTimerHandler() const
{
	CONTEXT("Processing check result files in '" + GetSpoolDir() + "'");

	Utility::Glob(GetSpoolDir() + "/c??????.ok", std::bind(&CheckResultReader::ProcessCheckResultFile, this, _1), GlobFile);
}

void CheckResultReader::ProcessCheckResultFile(const String& path) const
{
	CONTEXT("Processing check result file '" + path + "'");

	String crfile = String(path.Begin(), path.End() - 3); /* Remove the ".ok" extension. */

	std::ifstream fp;
	fp.exceptions(std::ifstream::badbit);
	fp.open(crfile.CStr());

	std::map<String, String> attrs;

	while (fp.good()) {
		std::string line;
		std::getline(fp, line);

		if (line.empty() || line[0] == '#')
			continue; /* Ignore comments and empty lines. */

		size_t pos = line.find_first_of('=');

		if (pos == std::string::npos)
			continue; /* Ignore invalid lines. */

		String key = line.substr(0, pos);
		String value = line.substr(pos + 1);

		attrs[key] = value;
	}

	/* Remove the checkresult files. */
	if (unlink(path.CStr()) < 0)
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("unlink")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(path));

	if (unlink(crfile.CStr()) < 0)
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("unlink")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(crfile));

	Checkable::Ptr checkable;

	Host::Ptr host = Host::GetByName(attrs["host_name"]);

	if (!host) {
		Log(LogWarning, "CheckResultReader")
			<< "Ignoring checkresult file for host '" << attrs["host_name"] << "': Host does not exist.";

		return;
	}

	if (attrs.find("service_description") != attrs.end()) {
		Service::Ptr service = host->GetServiceByShortName(attrs["service_description"]);

		if (!service) {
			Log(LogWarning, "CheckResultReader")
				<< "Ignoring checkresult file for host '" << attrs["host_name"]
				<< "', service '" << attrs["service_description"] << "': Service does not exist.";

			return;
		}

		checkable = service;
	} else
		checkable = host;

	CheckResult::Ptr result = new CheckResult();
	String output = CompatUtility::UnEscapeString(attrs["output"]);
	std::pair<String, Value> co = PluginUtility::ParseCheckOutput(output);
	result->SetOutput(co.first);
	result->SetPerformanceData(PluginUtility::SplitPerfdata(co.second));
	result->SetState(PluginUtility::ExitStatusToState(Convert::ToLong(attrs["return_code"])));

	if (attrs.find("start_time") != attrs.end())
		result->SetExecutionStart(Convert::ToDouble(attrs["start_time"]));
	else
		result->SetExecutionStart(Utility::GetTime());

	if (attrs.find("finish_time") != attrs.end())
		result->SetExecutionEnd(Convert::ToDouble(attrs["finish_time"]));
	else
		result->SetExecutionEnd(result->GetExecutionStart());

	checkable->ProcessCheckResult(result);

	Log(LogDebug, "CheckResultReader")
		<< "Processed checkresult file for object '" << checkable->GetName() << "'";

	/* Reschedule the next check. The side effect of this is that for as long
	 * as we receive check result files for a host/service we won't execute any
	 * active checks. */
	checkable->SetNextCheck(Utility::GetTime() + checkable->GetCheckInterval());
}
