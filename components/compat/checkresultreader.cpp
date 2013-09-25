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

#include "compat/checkresultreader.h"
#include "icinga/service.h"
#include "icinga/pluginchecktask.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/convert.h"
#include "base/application.h"
#include "base/utility.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <fstream>

using namespace icinga;

REGISTER_TYPE(CheckResultReader);

/**
 * @threadsafety Always.
 */
void CheckResultReader::Start(void)
{
	m_ReadTimer = boost::make_shared<Timer>();
	m_ReadTimer->OnTimerExpired.connect(boost::bind(&CheckResultReader::ReadTimerHandler, this));
	m_ReadTimer->SetInterval(5);
	m_ReadTimer->Start();
}

/**
 * @threadsafety Always.
 */
String CheckResultReader::GetSpoolDir(void) const
{
	if (!m_SpoolDir.IsEmpty())
		return m_SpoolDir;
	else
		return Application::GetLocalStateDir() + "/lib/icinga2/spool/checkresults/";
}

/**
 * @threadsafety Always.
 */
void CheckResultReader::ReadTimerHandler(void) const
{
	Utility::Glob(GetSpoolDir() + "/c??????.ok", boost::bind(&CheckResultReader::ProcessCheckResultFile, this, _1));
}

void CheckResultReader::ProcessCheckResultFile(const String& path) const
{
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
	(void)unlink(path.CStr());
	(void)unlink(crfile.CStr());

	Host::Ptr host = Host::GetByName(attrs["host_name"]);

	if (!host) {
		Log(LogWarning, "compat", "Ignoring checkresult file for host '" + attrs["host_name"] +
		    "': Host does not exist.");

		return;
	}

	Service::Ptr service = host->GetServiceByShortName(attrs["service_description"]);

	if (!service) {
		Log(LogWarning, "compat", "Ignoring checkresult file for host '" + attrs["host_name"] +
		    "', service '" + attrs["service_description"] + "': Service does not exist.");

		return;
	}

	Dictionary::Ptr result = PluginCheckTask::ParseCheckOutput(attrs["output"]);
	result->Set("state", PluginCheckTask::ExitStatusToState(Convert::ToLong(attrs["return_code"])));
	result->Set("execution_start", Convert::ToDouble(attrs["start_time"]));
	result->Set("execution_end", Convert::ToDouble(attrs["finish_time"]));
	result->Set("active", 1);

	service->ProcessCheckResult(result);

	Log(LogDebug, "compat", "Processed checkresult file for host '" + attrs["host_name"] +
		    "', service '" + attrs["service_description"] + "'");

	{
		ObjectLock olock(service);

		/* Reschedule the next check. The side effect of this is that for as long
		 * as we receive check result files for a service we won't execute any
		 * active checks. */
		service->SetNextCheck(Utility::GetTime() + service->GetCheckInterval());
	}
}

void CheckResultReader::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config)
		bag->Set("spool_dir", m_SpoolDir);
}

void CheckResultReader::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config)
		m_SpoolDir = bag->Get("spool_dir");
}
