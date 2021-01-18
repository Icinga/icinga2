/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "config/configcompiler.hpp"
#include "base/tlsutility.hpp"
#include "base/json.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"
#include "base/shared.hpp"
#include "base/utility.hpp"
#include <fstream>
#include <iomanip>
#include <thread>

using namespace icinga;

REGISTER_APIFUNCTION(Update, config, &ApiListener::ConfigUpdateHandler);

std::mutex ApiListener::m_ConfigSyncStageLock;

/**
 * Entrypoint for updating all authoritative configs from /etc/zones.d, packages, etc.
 * into var/lib/icinga2/api/zones
 */
void ApiListener::SyncLocalZoneDirs() const
{
	for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
		try {
			SyncLocalZoneDir(zone);
		} catch (const std::exception&) {
			continue;
		}
	}
}

/**
 * Sync a zone directory where we have an authoritative copy (zones.d, packages, etc.)
 *
 * This function collects the registered zone config dirs from
 * the config compiler and reads the file content into the config
 * information structure.
 *
 * Returns early when there are no updates.
 *
 * @param zone Pointer to the zone object being synced.
 */
void ApiListener::SyncLocalZoneDir(const Zone::Ptr& zone) const
{
	if (!zone)
		return;

	ConfigDirInformation newConfigInfo;
	newConfigInfo.UpdateV1 = new Dictionary();
	newConfigInfo.UpdateV2 = new Dictionary();
	newConfigInfo.Checksums = new Dictionary();

	String zoneName = zone->GetName();

	// Load registered zone paths, e.g. '_etc', '_api' and user packages.
	for (const ZoneFragment& zf : ConfigCompiler::GetZoneDirs(zoneName)) {
		ConfigDirInformation newConfigPart = LoadConfigDir(zf.Path);

		// Config files '*.conf'.
		{
			ObjectLock olock(newConfigPart.UpdateV1);
			for (const Dictionary::Pair& kv : newConfigPart.UpdateV1) {
				String path = "/" + zf.Tag + kv.first;

				newConfigInfo.UpdateV1->Set(path, kv.second);
				newConfigInfo.Checksums->Set(path, GetChecksum(kv.second));
			}
		}

		// Meta files.
		{
			ObjectLock olock(newConfigPart.UpdateV2);
			for (const Dictionary::Pair& kv : newConfigPart.UpdateV2) {
				String path = "/" + zf.Tag + kv.first;

				newConfigInfo.UpdateV2->Set(path, kv.second);
				newConfigInfo.Checksums->Set(path, GetChecksum(kv.second));
			}
		}
	}

	size_t sumUpdates = newConfigInfo.UpdateV1->GetLength() + newConfigInfo.UpdateV2->GetLength();

	// Return early if there are no updates.
	if (sumUpdates == 0)
		return;

	String productionZonesDir = GetApiZonesDir() + zoneName;

	Log(LogInformation, "ApiListener")
		<< "Copying " << sumUpdates << " zone configuration files for zone '" << zoneName << "' to '" << productionZonesDir << "'.";

	// Purge files to allow deletion via zones.d.
	if (Utility::PathExists(productionZonesDir))
		Utility::RemoveDirRecursive(productionZonesDir);

	Utility::MkDirP(productionZonesDir, 0700);

	// Copy content and add additional meta data.
	size_t numBytes = 0;

	/* Note: We cannot simply copy directories here.
	 *
	 * Zone directories are registered from everywhere and we already
	 * have read their content into memory with LoadConfigDir().
	 */
	Dictionary::Ptr newConfig = MergeConfigUpdate(newConfigInfo);

	{
		ObjectLock olock(newConfig);

		for (const Dictionary::Pair& kv : newConfig) {
			String dst = productionZonesDir + "/" + kv.first;

			Utility::MkDirP(Utility::DirName(dst), 0755);

			Log(LogInformation, "ApiListener")
				<< "Updating configuration file: " << dst;

			String content = kv.second;

			std::ofstream fp(dst.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);

			fp << content;
			fp.close();

			numBytes += content.GetLength();
		}
	}

	// Additional metadata.
	String tsPath = productionZonesDir + "/.timestamp";

	if (!Utility::PathExists(tsPath)) {
		std::ofstream fp(tsPath.CStr(), std::ofstream::out | std::ostream::trunc);

		fp << std::fixed << Utility::GetTime();
		fp.close();
	}

	String authPath = productionZonesDir + "/.authoritative";

	if (!Utility::PathExists(authPath)) {
		std::ofstream fp(authPath.CStr(), std::ofstream::out | std::ostream::trunc);
		fp.close();
	}

	// Checksums.
	String checksumsPath = productionZonesDir + "/.checksums";

	if (Utility::PathExists(checksumsPath))
		Utility::Remove(checksumsPath);

	std::ofstream fp(checksumsPath.CStr(), std::ofstream::out | std::ostream::trunc);

	fp << std::fixed << JsonEncode(newConfigInfo.Checksums);
	fp.close();

	Log(LogNotice, "ApiListener")
		<< "Updated meta data for cluster config sync. Checksum: '" << checksumsPath
		<< "', timestamp: '" << tsPath << "', auth: '" << authPath << "'.";
}

/**
 * Entrypoint for sending a file based config update to a cluster client.
 * This includes security checks for zone relations.
 * Loads the zone config files where this client belongs to
 * and sends the 'config::Update' JSON-RPC message.
 *
 * @param aclient Connected JSON-RPC client.
 */
void ApiListener::SendConfigUpdate(const JsonRpcConnection::Ptr& aclient)
{
	Endpoint::Ptr endpoint = aclient->GetEndpoint();
	ASSERT(endpoint);

	Zone::Ptr clientZone = endpoint->GetZone();
	Zone::Ptr localZone = Zone::GetLocalZone();

	// Don't send config updates to parent zones
	if (!clientZone->IsChildOf(localZone))
		return;

	Dictionary::Ptr configUpdateV1 = new Dictionary();
	Dictionary::Ptr configUpdateV2 = new Dictionary();
	Dictionary::Ptr configUpdateChecksums = new Dictionary(); // new since 2.11

	String zonesDir = GetApiZonesDir();

	for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
		String zoneName = zone->GetName();
		String zoneDir = zonesDir + zoneName;

		// Only sync child and global zones.
		if (!zone->IsChildOf(clientZone) && !zone->IsGlobal())
			continue;

		// Zone was configured, but there's no configuration directory.
		if (!Utility::PathExists(zoneDir))
			continue;

		Log(LogInformation, "ApiListener")
			<< "Syncing configuration files for " << (zone->IsGlobal() ? "global " : "")
			<< "zone '" << zoneName << "' to endpoint '" << endpoint->GetName() << "'.";

		ConfigDirInformation config = LoadConfigDir(zoneDir);

		configUpdateV1->Set(zoneName, config.UpdateV1);
		configUpdateV2->Set(zoneName, config.UpdateV2);
		configUpdateChecksums->Set(zoneName, config.Checksums); // new since 2.11
	}

	Dictionary::Ptr message = new Dictionary({
		{ "jsonrpc", "2.0" },
		{ "method", "config::Update" },
		{ "params", new Dictionary({
			{ "update", configUpdateV1 },
			{ "update_v2", configUpdateV2 },	// Since 2.4.2.
			{ "checksums", configUpdateChecksums } 	// Since 2.11.0.
		}) }
	});

	aclient->SendMessage(message);
}

static bool CompareTimestampsConfigChange(const Dictionary::Ptr& productionConfig, const Dictionary::Ptr& receivedConfig,
	const String& stageConfigZoneDir)
{
	double productionTimestamp;
	double receivedTimestamp;

	// Missing production timestamp means that something really broke. Always trigger a config change then.
	if (!productionConfig->Contains("/.timestamp"))
		productionTimestamp = 0;
	else
		productionTimestamp = productionConfig->Get("/.timestamp");

	// Missing received config timestamp means that something really broke. Always trigger a config change then.
	if (!receivedConfig->Contains("/.timestamp"))
		receivedTimestamp = Utility::GetTime() + 10;
	else
		receivedTimestamp = receivedConfig->Get("/.timestamp");

	bool configChange;

	// Skip update if our configuration files are more recent.
	if (productionTimestamp >= receivedTimestamp) {

		Log(LogInformation, "ApiListener")
			<< "Our production configuration is more recent than the received configuration update."
			<< " Ignoring configuration file update for path '" << stageConfigZoneDir << "'. Current timestamp '"
			<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", productionTimestamp) << "' ("
			<< std::fixed << std::setprecision(6) << productionTimestamp
			<< ") >= received timestamp '"
			<< Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", receivedTimestamp) << "' ("
			<< receivedTimestamp << ").";

		configChange = false;

	} else {
		configChange = true;
	}

	// Update the .timestamp file inside the staging directory.
	String tsPath = stageConfigZoneDir + "/.timestamp";

	if (!Utility::PathExists(tsPath)) {
		std::ofstream fp(tsPath.CStr(), std::ofstream::out | std::ostream::trunc);
		fp << std::fixed << receivedTimestamp;
		fp.close();
	}

	return configChange;
}

/**
 * Registered handler when a new config::Update message is received.
 *
 * Checks destination and permissions first, locks the transaction and analyses the update.
 * The newly received configuration is not copied to production immediately,
 * but into the staging directory first.
 * Last, the async validation and restart is triggered.
 *
 * @param origin Where this message came from.
 * @param params Message parameters including the config updates.
 * @returns Empty, required by the interface.
 */
Value ApiListener::ConfigUpdateHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	// Verify permissions and trust relationship.
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

	std::thread([origin, params, listener]() {
		try {
			listener->HandleConfigUpdate(origin, params);
		} catch (const std::exception& ex) {
			auto msg ("Exception during config sync: " + DiagnosticInformation(ex));

			Log(LogCritical, "ApiListener") << msg;
			listener->UpdateLastFailedZonesStageValidation(msg);
		}
	}).detach();
	return Empty;
}

void ApiListener::HandleConfigUpdate(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	/* Only one transaction is allowed, concurrent message handlers need to wait.
	 * This affects two parent endpoints sending the config in the same moment.
	 */
	std::lock_guard<std::mutex> lock(m_ConfigSyncStageLock);

	String apiZonesStageDir = GetApiZonesStageDir();
	String fromEndpointName = origin->FromClient->GetEndpoint()->GetName();
	String fromZoneName = GetFromZoneName(origin->FromZone);

	Log(LogInformation, "ApiListener")
		<< "Applying config update from endpoint '" << fromEndpointName
		<< "' of zone '" << fromZoneName << "'.";

	// Config files.
	Dictionary::Ptr updateV1 = params->Get("update");
	// Meta data files: .timestamp, etc.
	Dictionary::Ptr updateV2 = params->Get("update_v2");

	// New since 2.11.0.
	Dictionary::Ptr checksums;

	if (params->Contains("checksums"))
		checksums = params->Get("checksums");

	bool configChange = false;

	// Keep track of the relative config paths for later validation and copying. TODO: Find a better algorithm.
	std::vector<String> relativePaths;

	/*
	 * We can and must safely purge the staging directory, as the difference is taken between
	 * runtime production config and newly received configuration.
	 * This is needed to not mix deleted/changed content between received and stage
	 * config.
	 */
	if (Utility::PathExists(apiZonesStageDir))
		Utility::RemoveDirRecursive(apiZonesStageDir);

	Utility::MkDirP(apiZonesStageDir, 0700);

	// Analyse and process the update.
	size_t count = 0;

	ObjectLock olock(updateV1);

	for (const Dictionary::Pair& kv : updateV1) {

		// Check for the configured zones.
		String zoneName = kv.first;
		Zone::Ptr zone = Zone::GetByName(zoneName);

		if (!zone) {
			Log(LogWarning, "ApiListener")
				<< "Ignoring config update from endpoint '" << fromEndpointName
				<< "' for unknown zone '" << zoneName << "'.";

			continue;
		}

		// Ignore updates where we have an authoritive copy in etc/zones.d, packages, etc.
		if (ConfigCompiler::HasZoneConfigAuthority(zoneName)) {
			Log(LogInformation, "ApiListener")
				<< "Ignoring config update from endpoint '" << fromEndpointName
				<< "' for zone '" << zoneName << "' because we have an authoritative version of the zone's config.";

			continue;
		}

		// Put the received configuration into our stage directory.
		String productionConfigZoneDir = GetApiZonesDir() + zoneName;
		String stageConfigZoneDir = GetApiZonesStageDir() + zoneName;

		Utility::MkDirP(productionConfigZoneDir, 0700);
		Utility::MkDirP(stageConfigZoneDir, 0700);

		// Merge the config information.
		ConfigDirInformation newConfigInfo;
		newConfigInfo.UpdateV1 = kv.second;

		// Load metadata.
		if (updateV2)
			newConfigInfo.UpdateV2 = updateV2->Get(kv.first);

		// Load checksums. New since 2.11.
		if (checksums)
			newConfigInfo.Checksums = checksums->Get(kv.first);

		// Load the current production config details.
		ConfigDirInformation productionConfigInfo = LoadConfigDir(productionConfigZoneDir);

		// Merge updateV1 and updateV2
		Dictionary::Ptr productionConfig = MergeConfigUpdate(productionConfigInfo);
		Dictionary::Ptr newConfig = MergeConfigUpdate(newConfigInfo);

		bool timestampChanged = false;

		if (CompareTimestampsConfigChange(productionConfig, newConfig, stageConfigZoneDir)) {
			timestampChanged = true;
		}

		/* If we have received 'checksums' via cluster message, go for it.
		 * Otherwise do the old timestamp dance for versions < 2.11.
		 */
		if (checksums) {
			Log(LogInformation, "ApiListener")
				<< "Received configuration for zone '" << zoneName << "' from endpoint '"
				<< fromEndpointName << "'. Comparing the timestamp and checksums.";

			if (timestampChanged) {

				if (CheckConfigChange(productionConfigInfo, newConfigInfo))
					configChange = true;
			}

		} else {
			/* Fallback to timestamp handling when the parent endpoint didn't send checks.
			 * This can happen when the satellite is 2.11 and the master is 2.10.
			 *
			 * TODO: Deprecate and remove this behaviour in 2.13+.
			 */

			Log(LogWarning, "ApiListener")
				<< "Received configuration update without checksums from parent endpoint "
				<< fromEndpointName << ". This behaviour is deprecated. Please upgrade the parent endpoint to 2.11+";

			if (timestampChanged) {
				configChange = true;
			}

			// Keep another hack when there's a timestamp file missing.
			{
				ObjectLock olock(newConfig);

				for (const Dictionary::Pair &kv : newConfig) {

					// This is super expensive with a string content comparison.
					if (productionConfig->Get(kv.first) != kv.second) {
						if (!Utility::Match("*/.timestamp", kv.first))
							configChange = true;
					}
				}
			}
		}

		// Dump the received configuration for this zone into the stage directory.
		size_t numBytes = 0;

		{
			ObjectLock olock(newConfig);

			for (const Dictionary::Pair& kv : newConfig) {

				/* Store the relative config file path for later validation and activation.
				 * IMPORTANT: Store this prior to any filters.
				 * */
				relativePaths.push_back(zoneName + "/" + kv.first);

				String path = stageConfigZoneDir + "/" + kv.first;

				if (Utility::Match("*.conf", path)) {
					Log(LogInformation, "ApiListener")
						<< "Stage: Updating received configuration file '" << path << "' for zone '" << zoneName << "'.";
				}

				// Parent nodes < 2.11 always send this, avoid this bug and deny its receival prior to writing it on disk.
				if (Utility::BaseName(path) == ".authoritative")
					continue;

				// Sync string content only.
				String content = kv.second;

				// Generate a directory tree (zones/1/2/3 might not exist yet).
				Utility::MkDirP(Utility::DirName(path), 0755);

				// Write the content to file.
				std::ofstream fp(path.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
				fp << content;
				fp.close();

				numBytes += content.GetLength();
			}
		}

		Log(LogInformation, "ApiListener")
			<< "Applying configuration file update for path '" << stageConfigZoneDir << "' ("
			<< numBytes << " Bytes).";

		if (timestampChanged) {
			// If the update removes a path, delete it on disk and signal a config change.
			ObjectLock xlock(productionConfig);

			for (const Dictionary::Pair& kv : productionConfig) {
				if (!newConfig->Contains(kv.first)) {
					configChange = true;

					String path = stageConfigZoneDir + "/" + kv.first;
					Utility::Remove(path);
				}
			}
		}

		count++;
	}

	/*
	 * We have processed all configuration files and stored them in the staging directory.
	 *
	 * We need to store them locally for later analysis. A config change means
	 * that we will validate the configuration in a separate process sandbox,
	 * and only copy the configuration to production when everything is ok.
	 *
	 * A successful validation also triggers the final restart.
	 */
	if (configChange) {
		Log(LogInformation, "ApiListener")
			<< "Received configuration updates (" << count << ") from endpoint '" << fromEndpointName
			<< "' are different to production, triggering validation and reload.";
		TryActivateZonesStage(relativePaths);
	} else {
		Log(LogInformation, "ApiListener")
			<< "Received configuration updates (" << count << ") from endpoint '" << fromEndpointName
			<< "' are equal to production, skipping validation and reload.";
		ClearLastFailedZonesStageValidation();
	}
}

/**
 * Spawns a new validation process with 'System.ZonesStageVarDir' set to override the config validation zone dirs with
 * our current stage. Then waits for the validation result and if it was successful, the configuration is copied from
 * stage to production and a restart is triggered. On validation failure, there is no restart and this is logged.
 *
 * The caller of this function must hold m_ConfigSyncStageLock.
 *
 * @param relativePaths Collected paths including the zone name, which are copied from stage to current directories.
 */
void ApiListener::TryActivateZonesStage(const std::vector<String>& relativePaths)
{
	VERIFY(Application::GetArgC() >= 1);

	/* Inherit parent process args. */
	Array::Ptr args = new Array({
		Application::GetExePath(Application::GetArgV()[0]),
	});

	for (int i = 1; i < Application::GetArgC(); i++) {
		String argV = Application::GetArgV()[i];

		if (argV == "-d" || argV == "--daemonize")
			continue;

		args->Add(argV);
	}

	args->Add("--validate");

	// Set the ZonesStageDir. This creates our own local chroot without any additional automated zone includes.
	args->Add("--define");
	args->Add("System.ZonesStageVarDir=" + GetApiZonesStageDir());

	Process::Ptr process = new Process(Process::PrepareCommand(args));
	process->SetTimeout(Application::GetReloadTimeout());

	process->Run();
	const ProcessResult& pr = process->WaitForResult();

	String apiZonesDir = GetApiZonesDir();
	String apiZonesStageDir = GetApiZonesStageDir();

	String logFile = apiZonesStageDir + "/startup.log";
	std::ofstream fpLog(logFile.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpLog << pr.Output;
	fpLog.close();

	String statusFile = apiZonesStageDir + "/status";
	std::ofstream fpStatus(statusFile.CStr(), std::ofstream::out | std::ostream::binary | std::ostream::trunc);
	fpStatus << pr.ExitStatus;
	fpStatus.close();

	// Validation went fine, copy stage and reload.
	if (pr.ExitStatus == 0) {
		Log(LogInformation, "ApiListener")
			<< "Config validation for stage '" << apiZonesStageDir << "' was OK, replacing into '" << apiZonesDir << "' and triggering reload.";

		// Purge production before copying stage.
		if (Utility::PathExists(apiZonesDir))
			Utility::RemoveDirRecursive(apiZonesDir);

		Utility::MkDirP(apiZonesDir, 0700);

		// Copy all synced configuration files from stage to production.
		for (const String& path : relativePaths) {
			if (!Utility::PathExists(apiZonesStageDir + path))
				continue;

			Log(LogInformation, "ApiListener")
				<< "Copying file '" << path << "' from config sync staging to production zones directory.";

			String stagePath = apiZonesStageDir + path;
			String currentPath = apiZonesDir + path;

			Utility::MkDirP(Utility::DirName(currentPath), 0700);

			Utility::CopyFile(stagePath, currentPath);
		}

		// Clear any failed deployment before
		ApiListener::Ptr listener = ApiListener::GetInstance();

		if (listener)
			listener->ClearLastFailedZonesStageValidation();

		Application::RequestRestart();

		// All good, return early.
		return;
	}

	// Error case.
	Log(LogCritical, "ApiListener")
		<< "Config validation failed for staged cluster config sync in '" << apiZonesStageDir
		<< "'. Aborting. Logs: '" << logFile << "'";

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (listener)
		listener->UpdateLastFailedZonesStageValidation(pr.Output);
}

/**
 * Update the structure from the last failed validation output.
 * Uses the current timestamp.
 *
 * @param log The process output from the config validation.
 */
void ApiListener::UpdateLastFailedZonesStageValidation(const String& log)
{
	Dictionary::Ptr lastFailedZonesStageValidation = new Dictionary({
		{ "log", log },
		{ "ts", Utility::GetTime() }
	});

	SetLastFailedZonesStageValidation(lastFailedZonesStageValidation);
}

/**
 * Clear the structure for the last failed reload.
 *
 */
void ApiListener::ClearLastFailedZonesStageValidation()
{
	SetLastFailedZonesStageValidation(Dictionary::Ptr());
}

/**
 * Generate a config checksum.
 *
 * @param content String content used for generating the checksum.
 * @returns The checksum as string.
 */
String ApiListener::GetChecksum(const String& content)
{
	return SHA256(content);
}

bool ApiListener::CheckConfigChange(const ConfigDirInformation& oldConfig, const ConfigDirInformation& newConfig)
{
	Dictionary::Ptr oldChecksums = oldConfig.Checksums;
	Dictionary::Ptr newChecksums = newConfig.Checksums;

	// TODO: Figure out whether normal users need this for debugging.
	Log(LogDebug, "ApiListener")
		<< "Checking for config change between stage and production. Old (" << oldChecksums->GetLength() << "): '"
		<< JsonEncode(oldChecksums)
		<< "' vs. new (" << newChecksums->GetLength() << "): '"
		<< JsonEncode(newChecksums) << "'.";

	/* Since internal files are synced here too, we can not depend on length.
	 * So we need to go through both checksum sets to cover the cases"everything is new" and "everything was deleted".
	 */
	{
		ObjectLock olock(oldChecksums);
		for (const Dictionary::Pair& kv : oldChecksums) {
			String path = kv.first;
			String oldChecksum = kv.second;

			/* Ignore internal files, especially .timestamp and .checksums.
			 *
			 * If we don't, this results in "always change" restart loops.
			 */
			if (Utility::Match("/.*", path)) {
				Log(LogDebug, "ApiListener")
					<< "Ignoring old internal file '" << path << "'.";

				continue;
			}

			Log(LogDebug, "ApiListener")
				<< "Checking " << path << " for old checksum: " << oldChecksum << ".";

			// Check if key exists first for more verbose logging.
			// Note: Don't do this later on.
			if (!newChecksums->Contains(path)) {
				Log(LogDebug, "ApiListener")
					<< "File '" << path << "' was deleted by remote.";

				return true;
			}

			String newChecksum = newChecksums->Get(path);

			if (newChecksum != kv.second) {
				Log(LogDebug, "ApiListener")
					<< "Path '" << path << "' doesn't match old checksum '"
					<< oldChecksum << "' with new checksum '" << newChecksum << "'.";

				return true;
			}
		}
	}

	{
		ObjectLock olock(newChecksums);
		for (const Dictionary::Pair& kv : newChecksums) {
			String path = kv.first;
			String newChecksum = kv.second;

			/* Ignore internal files, especially .timestamp and .checksums.
			 *
			 * If we don't, this results in "always change" restart loops.
			 */
			if (Utility::Match("/.*", path)) {
				Log(LogDebug, "ApiListener")
					<< "Ignoring new internal file '" << path << "'.";

				continue;
			}

			Log(LogDebug, "ApiListener")
				<< "Checking " << path << " for new checksum: " << newChecksum << ".";

			// Check if the checksum exists, checksums in both sets have already been compared
			if (!oldChecksums->Contains(path)) {
				Log(LogDebug, "ApiListener")
					<< "File '" << path << "' was added by remote.";

				return true;
			}
		}
	}

	return false;
}

/**
 * Load the given config dir and read their file content into the config structure.
 *
 * @param dir Path to the config directory.
 * @returns ConfigDirInformation structure.
 */
ConfigDirInformation ApiListener::LoadConfigDir(const String& dir)
{
	ConfigDirInformation config;
	config.UpdateV1 = new Dictionary();
	config.UpdateV2 = new Dictionary();
	config.Checksums = new Dictionary();

	Utility::GlobRecursive(dir, "*", [&config, dir](const String& file) { ConfigGlobHandler(config, dir, file); }, GlobFile);
	return config;
}

/**
 * Read the given file and store it in the config information structure.
 * Callback function for Glob().
 *
 * @param config Reference to the config information object.
 * @param path File path.
 * @param file Full file name.
 */
void ApiListener::ConfigGlobHandler(ConfigDirInformation& config, const String& path, const String& file)
{
	// Avoid loading the authoritative marker for syncs at all cost.
	if (Utility::BaseName(file) == ".authoritative")
		return;

	CONTEXT("Creating config update for file '" + file + "'");

	Log(LogNotice, "ApiListener")
		<< "Creating config update for file '" << file << "'.";

	std::ifstream fp(file.CStr(), std::ifstream::binary);
	if (!fp)
		return;

	String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());

	Dictionary::Ptr update;
	String relativePath = file.SubStr(path.GetLength());

	/*
	 * 'update' messages contain conf files. 'update_v2' syncs everything else (.timestamp).
	 *
	 * **Keep this intact to stay compatible with older clients.**
	 */
	String sanitizedContent = Utility::ValidateUTF8(content);

	if (Utility::Match("*.conf", file)) {
		update = config.UpdateV1;

		// Configuration files should be automatically sanitized with UTF8.
		update->Set(relativePath, sanitizedContent);
	} else {
		update = config.UpdateV2;

		/*
		 * Ensure that only valid UTF8 content is being read for the cluster config sync.
		 * Binary files are not supported when wrapped into JSON encoded messages.
		 * Rationale: https://github.com/Icinga/icinga2/issues/7382
		 */
		if (content != sanitizedContent) {
			Log(LogCritical, "ApiListener")
				<< "Ignoring file '" << file << "' for cluster config sync: Does not contain valid UTF8. Binary files are not supported.";
			return;
		}

		update->Set(relativePath, content);
	}

	/* Calculate a checksum for each file (and a global one later).
	 *
	 * IMPORTANT: Ignore the .authoritative file above, this must not be synced.
	 * */
	config.Checksums->Set(relativePath, GetChecksum(content));
}

/**
 * Compatibility helper for merging config update v1 and v2 into a global result.
 *
 * @param config Config information structure.
 * @returns Dictionary which holds the merged information.
 */
Dictionary::Ptr ApiListener::MergeConfigUpdate(const ConfigDirInformation& config)
{
	Dictionary::Ptr result = new Dictionary();

	if (config.UpdateV1)
		config.UpdateV1->CopyTo(result);

	if (config.UpdateV2)
		config.UpdateV2->CopyTo(result);

	return result;
}
