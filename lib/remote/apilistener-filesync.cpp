/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "config/configcompiler.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/utility.hpp"
#include <fstream>
#include <iomanip>

using namespace icinga;

REGISTER_APIFUNCTION(Update, config, &ApiListener::ConfigUpdateHandler);

boost::mutex ApiListener::m_ConfigSyncStageLock;

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

bool ApiListener::UpdateConfigDir(const ConfigDirInformation& oldConfigInfo, const ConfigDirInformation& newConfigInfo,
	const String& configDir, const String& zoneName, std::vector<String>& relativePaths, bool authoritative)
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

				/* Store the relative config file path for later. */
				relativePaths.push_back(zoneName + "/" + kv.first);

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

	/* Update with staging information TODO - use `authoritative` as flag. */
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

	String oldDir = Configuration::DataDir + "/api/zones/" + zone->GetName();

	Log(LogInformation, "ApiListener")
		<< "Copying " << sumUpdates << " zone configuration files for zone '" << zone->GetName() << "' to '" << oldDir << "'.";

	Utility::MkDirP(oldDir, 0700);

	ConfigDirInformation oldConfigInfo = LoadConfigDir(oldDir);

	std::vector<String> relativePaths;
	UpdateConfigDir(oldConfigInfo, newConfigInfo, oldDir, zone->GetName(), relativePaths, true);
}

void ApiListener::SyncZoneDirs() const
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

	String zonesDir = Configuration::DataDir + "/api/zones";

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

	Dictionary::Ptr message = new Dictionary({
		{ "jsonrpc", "2.0" },
		{ "method", "config::Update" },
		{ "params", new Dictionary({
			{ "update", configUpdateV1 },
			{ "update_v2", configUpdateV2 }
		}) }
	});

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

	/* Only one transaction is allowed, concurrent message handlers need to wait.
	 * This affects two parent endpoints sending the config in the same moment.
	 */
	boost::mutex::scoped_lock lock(m_ConfigSyncStageLock);

	Log(LogInformation, "ApiListener")
		<< "Applying config update from endpoint '" << origin->FromClient->GetEndpoint()->GetName()
		<< "' of zone '" << GetFromZoneName(origin->FromZone) << "'.";

	Dictionary::Ptr updateV1 = params->Get("update");
	Dictionary::Ptr updateV2 = params->Get("update_v2");

	bool configChange = false;
	std::vector<String> relativePaths;

	ObjectLock olock(updateV1);
	for (const Dictionary::Pair& kv : updateV1) {

		/* Check for the configured zones. */
		String zoneName = kv.first;
		Zone::Ptr zone = Zone::GetByName(zoneName);

		if (!zone) {
			Log(LogWarning, "ApiListener")
				<< "Ignoring config update for unknown zone '" << zoneName << "'.";
			continue;
		}

		/* Whether we already have configuration in zones.d. */
		if (ConfigCompiler::HasZoneConfigAuthority(zoneName)) {
			Log(LogWarning, "ApiListener")
				<< "Ignoring config update for zone '" << zoneName << "' because we have an authoritative version of the zone's config.";
			continue;
		}

		/* Put the received configuration into our stage directory. */
		String currentConfigDir = GetApiZonesDir() + zoneName;
		String stageConfigDir = GetApiZonesStageDir() + zoneName;

		Utility::MkDirP(currentConfigDir, 0700);
		Utility::MkDirP(stageConfigDir, 0700);

		ConfigDirInformation newConfigInfo;
		newConfigInfo.UpdateV1 = kv.second;

		if (updateV2)
			newConfigInfo.UpdateV2 = updateV2->Get(kv.first);

		Dictionary::Ptr newConfig = kv.second;
		ConfigDirInformation currentConfigInfo = LoadConfigDir(currentConfigDir);

		/* Move the received configuration into our stage directory first. */
		if (UpdateConfigDir(currentConfigInfo, newConfigInfo, stageConfigDir, zoneName, relativePaths, false))
			configChange = true;
	}

	if (configChange) {
		/* Spawn a validation process. On success, move the staged configuration
		 * into production and restart.
		 */
		AsyncTryActivateZonesStage(GetApiZonesStageDir(), GetApiZonesDir(), relativePaths, true);
	}

	return Empty;
}

void ApiListener::TryActivateZonesStageCallback(const ProcessResult& pr,
	const String& stageConfigDir, const String& currentConfigDir,
	const std::vector<String>& relativePaths, bool reload)
{
	String logFile = GetApiZonesStageDir() + "/startup.log";
	std::ofstream fpLog(logFile.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpLog << pr.Output;
	fpLog.close();

	String statusFile = GetApiZonesStageDir() + "/status";
	std::ofstream fpStatus(statusFile.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpStatus << pr.ExitStatus;
	fpStatus.close();

	/* validation went fine, copy stage and reload */
	if (pr.ExitStatus == 0) {
		for (const String& path : relativePaths) {
			/* TODO: Better error handling with existing files. */
			Log(LogCritical, "ApiListener")
				<< "Copying file '" << path << "' from config sync staging to production directory.";

			Utility::CopyFile(GetApiZonesStageDir() + path, GetApiZonesDir() + path);
		}

		if (reload)
			Application::RequestRestart();
	} else {
		Log(LogCritical, "ApiListener")
			<< "Config validation failed for staged cluster config sync. Stage not put in production, aborting.";
	}
}

void ApiListener::AsyncTryActivateZonesStage(const String& stageConfigDir, const String& currentConfigDir,
	const std::vector<String>& relativePaths, bool reload)
{
	VERIFY(Application::GetArgC() >= 1);

	// prepare arguments
	Array::Ptr args = new Array({
		Application::GetExePath(Application::GetArgV()[0]),
		"daemon",
		"--validate",
		"--define",
		"ZonesDir=" + GetApiZonesStageDir()
	});

	Process::Ptr process = new Process(Process::PrepareCommand(args));
	process->SetTimeout(300);
	process->Run(std::bind(&TryActivateZonesStageCallback, _1, stageConfigDir, currentConfigDir, relativePaths, reload));
}
