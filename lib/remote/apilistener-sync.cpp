/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "remote/apilistener.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include <boost/foreach.hpp>
#include <fstream>

using namespace icinga;

bool ApiListener::IsConfigMaster(const Zone::Ptr& zone) const
{
	String path = Application::GetZonesDir() + "/" + zone->GetName();
	return Utility::PathExists(path);
}

void ApiListener::ConfigGlobHandler(const Dictionary::Ptr& config, const String& path, const String& file)
{
	CONTEXT("Creating config update for file '" + file + "'");

	std::ifstream fp(file.CStr());
	if (!fp)
		return;

	String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());
	config->Set(file.SubStr(path.GetLength()), content);
}

void ApiListener::SyncZoneDir(const Zone::Ptr& zone) const
{
	Log(LogInformation, "remote", "Syncing zone: " + zone->GetName());

	String dirNew = Application::GetZonesDir() + "/" + zone->GetName();
	String dirOld = Application::GetLocalStateDir() + "/lib/icinga2/api/zones/" + zone->GetName();

#ifndef _WIN32
	if (mkdir(dirOld.CStr(), 0700) < 0 && errno != EEXIST) {
#else /*_ WIN32 */
	if (mkdir(dirOld.CStr()) < 0 && errno != EEXIST) {
#endif /* _WIN32 */
		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("mkdir")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(dirOld));
	}

	Dictionary::Ptr configNew = make_shared<Dictionary>();
	Utility::GlobRecursive(dirNew, "*.conf", boost::bind(&ApiListener::ConfigGlobHandler, configNew, dirNew, _1), GlobFile);

	Dictionary::Ptr configOld = make_shared<Dictionary>();
	Utility::GlobRecursive(dirOld, "*.conf", boost::bind(&ApiListener::ConfigGlobHandler, configOld, dirOld, _1), GlobFile);

	BOOST_FOREACH(const Dictionary::Pair& kv, configNew) {
		if (configOld->Get(kv.first) != kv.second) {
			String path = dirOld + "/" + kv.first;
			Log(LogInformation, "remote", "Updating configuration file: " + path);

			std::ofstream fp(path.CStr(), std::ofstream::out | std::ostream::trunc);
			fp << kv.second;
			fp.close();
		}
	}

	BOOST_FOREACH(const Dictionary::Pair& kv, configOld) {
		if (!configNew->Contains(kv.first)) {
			String path = dirOld + "/" + kv.first;
			(void) unlink(path.CStr());
		}
	}
}

void ApiListener::SyncZoneDirs(void) const
{
	BOOST_FOREACH(const Zone::Ptr& zone, DynamicType::GetObjects<Zone>()) {
		if (!IsConfigMaster(zone))
			continue;

		SyncZoneDir(zone);
	}

	bool configChange = false;

	// TODO: remove configuration files for zones which don't exist anymore (i.e. don't have a Zone object)

	if (configChange) {
		Log(LogInformation, "remote", "Restarting after configuration change.");
		Application::RequestRestart();
	}
}
