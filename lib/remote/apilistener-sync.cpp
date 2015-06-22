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

#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "base/dynamictype.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include <boost/foreach.hpp>
#include <fstream>
#include <iomanip>

using namespace icinga;

REGISTER_APIFUNCTION(Update, config, &ApiListener::ConfigUpdateHandler);

bool ApiListener::IsConfigMaster(const Zone::Ptr& zone)
{
	String path = Application::GetZonesDir() + "/" + zone->GetName();
	return Utility::PathExists(path);
}

void ApiListener::ConfigGlobHandler(Dictionary::Ptr& config, const String& path, const String& file)
{
	CONTEXT("Creating config update for file '" + file + "'");

	Log(LogNotice, "ApiListener")
	    << "Creating config update for file '" << file << "'";

	std::ifstream fp(file.CStr(), std::ifstream::binary);
	if (!fp)
		return;

	String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());
	config->Set(file.SubStr(path.GetLength()), content);
}

Dictionary::Ptr ApiListener::LoadConfigDir(const String& dir)
{
	Dictionary::Ptr config = new Dictionary();
	Utility::GlobRecursive(dir, "*.conf", boost::bind(&ApiListener::ConfigGlobHandler, boost::ref(config), dir, _1), GlobFile);
	return config;
}

bool ApiListener::UpdateConfigDir(const Dictionary::Ptr& oldConfig, const Dictionary::Ptr& newConfig, const String& configDir, bool authoritative)
{
	bool configChange = false;

	if (oldConfig->Contains(".timestamp") && newConfig->Contains(".timestamp")) {
		double oldTS = Convert::ToDouble(oldConfig->Get(".timestamp"));
		double newTS = Convert::ToDouble(newConfig->Get(".timestamp"));

		/* skip update if our config is newer */
		if (oldTS <= newTS)
			return false;
	}

	{
		ObjectLock olock(newConfig);
		BOOST_FOREACH(const Dictionary::Pair& kv, newConfig) {
			if (oldConfig->Get(kv.first) != kv.second) {
				configChange = true;

				String path = configDir + "/" + kv.first;
				Log(LogInformation, "ApiListener")
				    << "Updating configuration file: " << path;

				//pass the directory and generate a dir tree, if not existing already
				Utility::MkDirP(Utility::DirName(path), 0755);
				std::ofstream fp(path.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
				fp << kv.second;
				fp.close();
			}
		}
	}

	ObjectLock xlock(oldConfig);
	BOOST_FOREACH(const Dictionary::Pair& kv, oldConfig) {
		if (!newConfig->Contains(kv.first)) {
			configChange = true;

			String path = configDir + "/" + kv.first;
			(void) unlink(path.CStr());
		}
	}

	String tsPath = configDir + "/.timestamp";
	if (!Utility::PathExists(tsPath)) {
		std::ofstream fp(tsPath.CStr(), std::ofstream::out | std::ostream::trunc);
		fp << std::fixed << Utility::GetTime();
		fp.close();
	}

	if (authoritative) {
		String authPath = configDir + "/.authoritative";
		if (!Utility::PathExists(authPath)) {
			std::ofstream fp(authPath.CStr(), std::ofstream::out | std::ostream::trunc);
			fp.close();
		}
	}

	return configChange;
}

void ApiListener::SyncZoneDir(const Zone::Ptr& zone) const
{
	String newDir = Application::GetZonesDir() + "/" + zone->GetName();
	String oldDir = Application::GetLocalStateDir() + "/lib/icinga2/api/zones/" + zone->GetName();

	Log(LogInformation, "ApiListener")
	    << "Copying zone configuration files from '" << newDir << "' to  '" << oldDir << "'.";

	if (!Utility::MkDir(oldDir, 0700)) {
		Log(LogCritical, "ApiListener")
		    << "mkdir() for path '" << oldDir << "' failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

		BOOST_THROW_EXCEPTION(posix_error()
			<< boost::errinfo_api_function("mkdir")
			<< boost::errinfo_errno(errno)
			<< boost::errinfo_file_name(oldDir));
	}

	Dictionary::Ptr newConfig = LoadConfigDir(newDir);
	Dictionary::Ptr oldConfig = LoadConfigDir(oldDir);

	UpdateConfigDir(oldConfig, newConfig, oldDir, true);
}

void ApiListener::SyncZoneDirs(void) const
{
	BOOST_FOREACH(const Zone::Ptr& zone, DynamicType::GetObjectsByType<Zone>()) {
		if (!IsConfigMaster(zone))
			continue;

		try {
			SyncZoneDir(zone);
		} catch (const std::exception&) {
			continue;
		}
	}
}

void ApiListener::SendConfigUpdate(const JsonRpcConnection::Ptr& aclient)
{
	Endpoint::Ptr endpoint = aclient->GetEndpoint();
	ASSERT(endpoint);

	Zone::Ptr azone = endpoint->GetZone();
	Zone::Ptr lzone = Zone::GetLocalZone();

	/* don't try to send config updates to our master */
	if (!azone->IsChildOf(lzone))
		return;

	Dictionary::Ptr configUpdate = new Dictionary();

	String zonesDir = Application::GetLocalStateDir() + "/lib/icinga2/api/zones";

	BOOST_FOREACH(const Zone::Ptr& zone, DynamicType::GetObjectsByType<Zone>()) {
		String zoneDir = zonesDir + "/" + zone->GetName();

		if (!zone->IsChildOf(azone) && !zone->IsGlobal()) {
			Log(LogNotice, "ApiListener")
			    << "Skipping sync for '" << zone->GetName() << "'. Not a child of zone '" << azone->GetName() << "'.";
			continue;
		}
		if (!Utility::PathExists(zoneDir)) {
			Log(LogNotice, "ApiListener")
			    << "Ignoring sync for '" << zone->GetName() << "'. Zone directory '" << zoneDir << "' does not exist.";
			continue;
		}

		if (zone->IsGlobal())
			Log(LogInformation, "ApiListener")
			    << "Syncing global zone '" << zone->GetName() << "'.";

		configUpdate->Set(zone->GetName(), LoadConfigDir(zonesDir + "/" + zone->GetName()));
	}

	Dictionary::Ptr params = new Dictionary();
	params->Set("update", configUpdate);

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "config::Update");
	message->Set("params", params);

	aclient->SendMessage(message);
}

Value ApiListener::ConfigUpdateHandler(const MessageOrigin& origin, const Dictionary::Ptr& params)
{
	if (!origin.FromClient->GetEndpoint() || (origin.FromZone && !Zone::GetLocalZone()->IsChildOf(origin.FromZone)))
		return Empty;

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener) {
		Log(LogCritical, "ApiListener", "No instance available.");
		return Empty;
	}

	if (!listener->GetAcceptConfig()) {
		Log(LogWarning, "ApiListener")
		    << "Ignoring config update. '" << listener->GetName() << "' does not accept config.";
		return Empty;
	}

	Dictionary::Ptr update = params->Get("update");

	bool configChange = false;

	ObjectLock olock(update);
	BOOST_FOREACH(const Dictionary::Pair& kv, update) {
		Zone::Ptr zone = Zone::GetByName(kv.first);

		if (!zone) {
			Log(LogWarning, "ApiListener")
			    << "Ignoring config update for unknown zone '" << kv.first << "'.";
			continue;
		}

		if (IsConfigMaster(zone)) {
			Log(LogWarning, "ApiListener")
			    << "Ignoring config update for zone '" << kv.first << "' because we have an authoritative version of the zone's config.";
			continue;
		}

		String oldDir = Application::GetLocalStateDir() + "/lib/icinga2/api/zones/" + zone->GetName();

		if (!Utility::MkDir(oldDir, 0700)) {
			Log(LogCritical, "ApiListener")
			    << "mkdir() for path '" << oldDir << "' failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";

			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("mkdir")
				<< boost::errinfo_errno(errno)
				<< boost::errinfo_file_name(oldDir));
		}

		Dictionary::Ptr newConfig = kv.second;
		Dictionary::Ptr oldConfig = LoadConfigDir(oldDir);

		if (UpdateConfigDir(oldConfig, newConfig, oldDir, false))
			configChange = true;
	}

	if (configChange) {
		Log(LogInformation, "ApiListener", "Restarting after configuration change.");
		Application::RequestRestart();
	}

	return Empty;
}
