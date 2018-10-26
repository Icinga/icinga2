/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include "config/configcompiler.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include <fstream>
#include <iomanip>

using namespace icinga;

REGISTER_APIFUNCTION(Update, config, &ApiListener::ConfigUpdateHandler);

void ApiListener::ConfigGlobHandler(ConfigDirInformation& config, const String& path, const String& file)
{
	CONTEXT("Creating config update for file '" + file + "'");

	Log(LogNotice, "ApiListener")
	    << "Creating config update for file '" << file << "'.";

	std::ifstream fp(file.CStr(), std::ifstream::binary);
	if (!fp)
		return;

	String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());

	Dictionary::Ptr update;

	if (Utility::Match("*.conf", file))
		update = config.UpdateV1;
	else
		update = config.UpdateV2;

	update->Set(file.SubStr(path.GetLength()), content);
}

Dictionary::Ptr ApiListener::MergeConfigUpdate(const ConfigDirInformation& config)
{
	Dictionary::Ptr result = new Dictionary();

	if (config.UpdateV1)
		config.UpdateV1->CopyTo(result);

	if (config.UpdateV2)
		config.UpdateV2->CopyTo(result);

	return result;
}

ConfigDirInformation ApiListener::LoadConfigDir(const String& dir)
{
	ConfigDirInformation config;
	config.UpdateV1 = new Dictionary();
	config.UpdateV2 = new Dictionary();
	Utility::GlobRecursive(dir, "*", std::bind(&ApiListener::ConfigGlobHandler, std::ref(config), dir, _1), GlobFile);
	return config;
}

bool ApiListener::UpdateConfigDir(const ConfigDirInformation& oldConfigInfo, const ConfigDirInformation& newConfigInfo, const String& configDir, bool authoritative)
{
	bool configChange = false;

	Dictionary::Ptr oldConfig = MergeConfigUpdate(oldConfigInfo);
	Dictionary::Ptr newConfig = MergeConfigUpdate(newConfigInfo);

	double oldTimestamp;

	if (!oldConfig->Contains("/.timestamp"))
		oldTimestamp = 0;
	else
		oldTimestamp = oldConfig->Get("/.timestamp");

	double newTimestamp;

	if (!newConfig->Contains("/.timestamp"))
		newTimestamp = Utility::GetTime();
	else
		newTimestamp = newConfig->Get("/.timestamp");

	/* skip update if our configuration files are more recent */
	if (oldTimestamp >= newTimestamp) {
		Log(LogNotice, "ApiListener")
		    << "Our configuration is more recent than the received configuration update."
		    << " Ignoring configuration file update for path '" << configDir << "'. Current timestamp '"
		    << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", oldTimestamp) << "' ("
		    << std::fixed << std::setprecision(6) << oldTimestamp
		    << ") >= received timestamp '"
		    << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", newTimestamp) << "' ("
		    << newTimestamp << ").";
		return false;
	}

	size_t numBytes = 0;

	{
		ObjectLock olock(newConfig);
		for (const Dictionary::Pair& kv : newConfig) {
			if (oldConfig->Get(kv.first) != kv.second) {
				if (!Utility::Match("*/.timestamp", kv.first))
					configChange = true;

				String path = configDir + "/" + kv.first;
				Log(LogInformation, "ApiListener")
				    << "Updating configuration file: " << path;

				/* Sync string content only. */
				String content = kv.second;

				/* Generate a directory tree (zones/1/2/3 might not exist yet). */
				Utility::MkDirP(Utility::DirName(path), 0755);
				std::ofstream fp(path.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
				fp << content;
				fp.close();

				numBytes += content.GetLength();
			}
		}
	}

	Log(LogInformation, "ApiListener")
	    << "Applying configuration file update for path '" << configDir << "' (" << numBytes << " Bytes). Received timestamp '"
	    << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", newTimestamp) << "' ("
	    << std::fixed << std::setprecision(6) << newTimestamp
	    << "), Current timestamp '"
	    << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", oldTimestamp) << "' ("
	    << oldTimestamp << ").";

	ObjectLock xlock(oldConfig);
	for (const Dictionary::Pair& kv : oldConfig) {
		if (!newConfig->Contains(kv.first)) {
			configChange = true;

			String path = configDir + "/" + kv.first;
			(void) unlink(path.CStr());
		}
	}

	String tsPath = configDir + "/.timestamp";
	if (!Utility::PathExists(tsPath)) {
		std::ofstream fp(tsPath.CStr(), std::ofstream::out | std::ostream::trunc);
		fp << std::fixed << newTimestamp;
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
	ConfigDirInformation newConfigInfo;
	newConfigInfo.UpdateV1 = new Dictionary();
	newConfigInfo.UpdateV2 = new Dictionary();

	for (const ZoneFragment& zf : ConfigCompiler::GetZoneDirs(zone->GetName())) {
		ConfigDirInformation newConfigPart = LoadConfigDir(zf.Path);

		{
			ObjectLock olock(newConfigPart.UpdateV1);
			for (const Dictionary::Pair& kv : newConfigPart.UpdateV1) {
				newConfigInfo.UpdateV1->Set("/" + zf.Tag + kv.first, kv.second);
			}
		}

		{
			ObjectLock olock(newConfigPart.UpdateV2);
			for (const Dictionary::Pair& kv : newConfigPart.UpdateV2) {
				newConfigInfo.UpdateV2->Set("/" + zf.Tag + kv.first, kv.second);
			}
		}
	}

	int sumUpdates = newConfigInfo.UpdateV1->GetLength() + newConfigInfo.UpdateV2->GetLength();

	if (sumUpdates == 0)
		return;

	String oldDir = Application::GetLocalStateDir() + "/lib/icinga2/api/zones/" + zone->GetName();

	Log(LogInformation, "ApiListener")
	    << "Copying " << sumUpdates << " zone configuration files for zone '" << zone->GetName() << "' to '" << oldDir << "'.";

	Utility::MkDirP(oldDir, 0700);

	ConfigDirInformation oldConfigInfo = LoadConfigDir(oldDir);

	UpdateConfigDir(oldConfigInfo, newConfigInfo, oldDir, true);
}

void ApiListener::SyncZoneDirs(void) const
{
	for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
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

	Dictionary::Ptr configUpdateV1 = new Dictionary();
	Dictionary::Ptr configUpdateV2 = new Dictionary();

	String zonesDir = Application::GetLocalStateDir() + "/lib/icinga2/api/zones";

	for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
		String zoneDir = zonesDir + "/" + zone->GetName();

		if (!zone->IsChildOf(azone) && !zone->IsGlobal())
			continue;

		if (!Utility::PathExists(zoneDir))
			continue;

		Log(LogInformation, "ApiListener")
		    << "Syncing configuration files for " << (zone->IsGlobal() ? "global " : "")
		    << "zone '" << zone->GetName() << "' to endpoint '" << endpoint->GetName() << "'.";

		ConfigDirInformation config = LoadConfigDir(zonesDir + "/" + zone->GetName());
		configUpdateV1->Set(zone->GetName(), config.UpdateV1);
		configUpdateV2->Set(zone->GetName(), config.UpdateV2);
	}

	Dictionary::Ptr params = new Dictionary();
	params->Set("update", configUpdateV1);
	params->Set("update_v2", configUpdateV2);

	Dictionary::Ptr message = new Dictionary();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "config::Update");
	message->Set("params", params);

	aclient->SendMessage(message);
}

Value ApiListener::ConfigUpdateHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	if (!origin->FromClient->GetEndpoint() || (origin->FromZone && !Zone::GetLocalZone()->IsChildOf(origin->FromZone)))
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

	Log(LogInformation, "ApiListener")
	    << "Applying config update from endpoint '" << origin->FromClient->GetEndpoint()->GetName()
	    << "' of zone '" << GetFromZoneName(origin->FromZone) << "'.";

	Dictionary::Ptr updateV1 = params->Get("update");
	Dictionary::Ptr updateV2 = params->Get("update_v2");

	bool configChange = false;

	ObjectLock olock(updateV1);
	for (const Dictionary::Pair& kv : updateV1) {
		Zone::Ptr zone = Zone::GetByName(kv.first);

		if (!zone) {
			Log(LogWarning, "ApiListener")
			    << "Ignoring config update for unknown zone '" << kv.first << "'.";
			continue;
		}

		if (ConfigCompiler::HasZoneConfigAuthority(kv.first)) {
			Log(LogWarning, "ApiListener")
			    << "Ignoring config update for zone '" << kv.first << "' because we have an authoritative version of the zone's config.";
			continue;
		}

		String oldDir = Application::GetLocalStateDir() + "/lib/icinga2/api/zones/" + zone->GetName();

		Utility::MkDirP(oldDir, 0700);

		ConfigDirInformation newConfigInfo;
		newConfigInfo.UpdateV1 = kv.second;

		if (updateV2)
			newConfigInfo.UpdateV2 = updateV2->Get(kv.first);

		Dictionary::Ptr newConfig = kv.second;
		ConfigDirInformation oldConfigInfo = LoadConfigDir(oldDir);

		if (UpdateConfigDir(oldConfigInfo, newConfigInfo, oldDir, false))
			configChange = true;
	}

	if (configChange) {
		Log(LogInformation, "ApiListener", "Restarting after configuration change.");
		Application::RequestRestart();
	}

	return Empty;
}
