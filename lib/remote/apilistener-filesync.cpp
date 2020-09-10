/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "config/configcompiler.hpp"
#include "base/object-packer.hpp"
#include "base/tlsutility.hpp"
#include "base/json.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/application.hpp"
#include "base/exception.hpp"
#include "base/shared.hpp"
#include "base/utility.hpp"
#include <algorithm>
#include <fstream>
#include <iomanip>
#include <thread>
#include <utility>

using namespace icinga;

REGISTER_APIFUNCTION(Update, config, &ApiListener::ConfigUpdateHandler);
REGISTER_APIFUNCTION(HaveZones, config, &ApiListener::ConfigHaveZonesHandler);
REGISTER_APIFUNCTION(WantZones, config, &ApiListener::ConfigWantZonesHandler);
REGISTER_APIFUNCTION(HaveFiles, config, &ApiListener::ConfigHaveFilesHandler);
REGISTER_APIFUNCTION(WantFiles, config, &ApiListener::ConfigWantFilesHandler);

boost::mutex ApiListener::m_ConfigSyncStageLock;

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
	Dictionary::Ptr contents = new Dictionary();
	double ts = 0;

	// Load registered zone paths, e.g. '_etc', '_api' and user packages.
	for (const ZoneFragment& zf : ConfigCompiler::GetZoneDirs(zoneName)) {
		ConfigDirInformation newConfigPart = LoadConfigDir(zf.Path, contents);

		Utility::GlobRecursive(zf.Path, "*", [&ts](const String& path) { ts = std::max(ts, Utility::GetModTime(path)); }, GlobFile | GlobDirectory);

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

	if (ts == 0) {
		// Syncing an empty zone every time hurts less than not syncing it at all.
		ts = Utility::GetTime();
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

		fp << std::fixed << ts;
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

	// Checksum.
	String checksumPath = productionZonesDir + "/.checksum";

	if (Utility::PathExists(checksumPath))
		Utility::Remove(checksumPath);

	{
		std::ofstream fp(checksumPath.CStr(), std::ofstream::out | std::ostream::trunc);

		fp << GetChecksum(PackObject(contents));
		fp.close();
	}

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

static Dictionary::Ptr AssembleZoneDeclaration(const String& zoneName)
{
	auto base (ApiListener::GetApiZonesDir() + zoneName + "/");
	Dictionary::Ptr declaration = new Dictionary();

	{
		auto file (base + ".timestamp");
		std::ifstream fp (file.CStr(), std::ifstream::binary);

		if (!fp) {
			return nullptr;
		}

		double ts;

		try {
			ts = Convert::ToDouble(String(std::istreambuf_iterator<char>(fp), std::istreambuf_iterator<char>()));
		} catch (const std::exception&) {
			return nullptr;
		}

		declaration->Set("timestamp", ts);
	}

	{
		auto file (base + ".checksum");
		std::ifstream fp (file.CStr(), std::ifstream::binary);

		if (!fp) {
			return nullptr;
		}

		String content;

		try {
			content = String(std::istreambuf_iterator<char>(fp), std::istreambuf_iterator<char>());
		} catch (const std::exception&) {
			return nullptr;
		}

		declaration->Set("checksum", content);
	}

	return std::move(declaration);
}

/**
 * Entrypoint for sending a file based config declaration to a cluster client.
 * This includes security checks for zone relations.
 * Loads the zone config files where this client belongs to
 * and sends the 'config::HasZones' JSON-RPC message.
 *
 * @param aclient Connected JSON-RPC client.
 */
void ApiListener::DeclareConfigUpdate(const JsonRpcConnection::Ptr& client)
{
	Endpoint::Ptr endpoint = client->GetEndpoint();
	ASSERT(endpoint);

	Zone::Ptr clientZone = endpoint->GetZone();

	// Don't send config declarations to parent zones
	if (!clientZone->IsChildOf(Zone::GetLocalZone())) {
		return;
	}

	Dictionary::Ptr declaration = new Dictionary();
	String zonesDir = GetApiZonesDir();

	for (auto& zone : ConfigType::GetObjectsByType<Zone>()) {
		String zoneName = zone->GetName();
		String zoneDir = zonesDir + zoneName;

		// Only declare child and global zones.
		if (!zone->IsChildOf(clientZone) && !zone->IsGlobal()) {
			continue;
		}

		// Zone was configured, but there's no configuration directory.
		if (!Utility::PathExists(zoneDir)) {
			continue;
		}

		auto perZone (AssembleZoneDeclaration(zoneName));

		if (!perZone) {
			// We likely just were upgraded to v2.13 and our non-authoritive config
			// hasn't been synced from the config master, yet.

			Log(LogNotice, "ApiListener")
				<< "Not informing endpoint '" << endpoint->GetName() << "' about "
				<< (zone->IsGlobal() ? "global " : "") << "zone '" << zoneName
				<< "' to due to yet missing/bad .timestamp/.checksum file in '" << zoneDir << "'.";

			continue;
		}

		Log(LogInformation, "ApiListener")
			<< "Informing endpoint '" << endpoint->GetName() << "' about "
			<< (zone->IsGlobal() ? "global " : "") << "zone '" << zoneName << "'.";

		declaration->Set(zoneName, perZone);
	}

	if (declaration->GetLength()) {
		client->SendMessage(new Dictionary({
			{ "jsonrpc", "2.0" },
			{ "method", "config::HaveZones" },
			{ "params", new Dictionary({
				{ "zones", declaration }
			}) }
		}));
	}
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

	std::thread([origin, params]() { HandleConfigUpdate(origin, params); }).detach();
	return Empty;
}

Value ApiListener::ConfigHaveZonesHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	auto endpoint (origin->FromClient->GetEndpoint());

	// Verify permissions and trust relationship.
	if (!endpoint || (origin->FromZone && !Zone::GetLocalZone()->IsChildOf(origin->FromZone))) {
		return Empty;
	}

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener) {
		Log(LogCritical, "ApiListener", "No instance available.");
		return Empty;
	}

	if (!listener->GetAcceptConfig()) {
		Log(LogWarning, "ApiListener")
			<< "Ignoring information about zones. '" << listener->GetName() << "' does not accept config.";
		return Empty;
	}

	auto fromEndpointName (origin->FromClient->GetEndpoint()->GetName());
	auto fromZoneName (GetFromZoneName(origin->FromZone));

	Log(LogInformation, "ApiListener")
		<< "Checking information about zones from endpoint '" << fromEndpointName
		<< "' of zone '" << fromZoneName << "'.";

	Dictionary::Ptr zones = params->Get("zones");
	Array::Ptr want = new Array();
	ObjectLock oLock (zones);

	for (auto& kv : zones) {
		if (!Zone::GetByName(kv.first)) {
			Log(LogWarning, "ApiListener")
				<< "Ignoring information about unknown zone '" << kv.first
				<< "' from endpoint '" << fromEndpointName << "'.";
			continue;
		}

		// Ignore files declarations where we have an authoritive copy in etc/zones.d, packages, etc.
		if (ConfigCompiler::HasZoneConfigAuthority(kv.first)) {
			Log(LogInformation, "ApiListener")
				<< "Ignoring information about zone '" << kv.first << "' from endpoint '" << fromEndpointName
				<< "' because we have an authoritative version of the zone's config.";
			continue;
		}

		{
			auto local (AssembleZoneDeclaration(kv.first));
			Dictionary::Ptr remote = kv.second;

			if (local && (local->Get("checksum") == remote->Get("checksum") || local->Get("timestamp") >= remote->Get("timestamp"))) {
				Log(LogInformation, "ApiListener")
					<< "Ignoring information about zone '" << kv.first
					<< "' from endpoint '" << fromEndpointName << "' because we're up-to-date.";
				continue;
			}
		}

		Log(LogInformation, "ApiListener")
			<< "Requesting actual zone '" << kv.first << "' from endpoint '" << fromEndpointName
			<< "' because compared to it we're not up-to-date.";

		want->Add(kv.first);
	}

	JsonRpcConnection::Ptr client;

	for (auto& conn : endpoint->GetClients()) {
		client = conn;
		break;
	}

	if (!client) {
		Log(LogNotice, "ApiListener")
			<< "Compared to '" << fromEndpointName
			<< "' not everything is up-to-date, but we're not connected.";
		return Empty;
	}

	client->SendMessage(new Dictionary({
		{ "jsonrpc", "2.0" },
		{ "method", "config::WantZones" },
		{ "params", new Dictionary({
			{ "zones", want }
		}) }
	}));

	return Empty;
}

Value ApiListener::ConfigHaveFilesHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	auto endpoint (origin->FromClient->GetEndpoint());

	// Verify permissions and trust relationship.
	if (!endpoint || (origin->FromZone && !Zone::GetLocalZone()->IsChildOf(origin->FromZone))) {
		return Empty;
	}

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener) {
		Log(LogCritical, "ApiListener", "No instance available.");
		return Empty;
	}

	if (!listener->GetAcceptConfig()) {
		Log(LogWarning, "ApiListener")
			<< "Ignoring information about files in zones. '" << listener->GetName() << "' does not accept config.";
		return Empty;
	}

	auto fromEndpointName (origin->FromClient->GetEndpoint()->GetName());
	auto fromZoneName (GetFromZoneName(origin->FromZone));

	Log(LogInformation, "ApiListener")
		<< "Checking information about files in zones from endpoint '" << fromEndpointName
		<< "' of zone '" << fromZoneName << "'.";

	Dictionary::Ptr checksums = params->Get("checksums");
	auto zonesDir (GetApiZonesDir());
	Dictionary::Ptr want = new Dictionary();
	ObjectLock oLock (checksums);

	for (auto& kv : checksums) {
		if (!Zone::GetByName(kv.first)) {
			Log(LogWarning, "ApiListener")
				<< "Ignoring information about files in unknown zone '" << kv.first
				<< "' from endpoint '" << fromEndpointName << "'.";
			continue;
		}

		// Ignore files declarations where we have an authoritive copy in etc/zones.d, packages, etc.
		if (ConfigCompiler::HasZoneConfigAuthority(kv.first)) {
			Log(LogInformation, "ApiListener")
				<< "Ignoring information about files in zone '" << kv.first << "' from endpoint '"
				<< fromEndpointName << "' because we have an authoritative version of the zone's config.";
			continue;
		}

		String checksumsPath = zonesDir + kv.first + "/.checksums";
		Dictionary::Ptr checksums;
		std::ifstream fp (checksumsPath.CStr(), std::ifstream::binary);

		if (fp) {
			checksums = JsonDecode(String(
				std::istreambuf_iterator<char>(fp), std::istreambuf_iterator<char>()
			));
		} else {
			checksums = new Dictionary();
		}

		Dictionary::Ptr havePerZone = kv.second;
		Array::Ptr wantPerZone = new Array();
		ObjectLock oLock (havePerZone);

		for (auto& kv : havePerZone) {
			if (kv.second != checksums->Get(kv.first)) {
				wantPerZone->Add(kv.first);
			}

			checksums->Remove(kv.first);
		}

		Log(LogNotice, "ApiListener")
			<< "Compared to the information from endpoint '" << fromEndpointName
			<< "' about files in zone '" << kv.first << "' there are " << wantPerZone->GetLength()
			<< " file(s) to fetch the content(s) of and " << checksums->GetLength() << " file(s) to remove.";

		if (!wantPerZone->GetLength() && !checksums->GetLength()) {
			continue;
		}

		want->Set(kv.first, wantPerZone);
	}

	if (!want->GetLength()) {
		Log(LogNotice, "ApiListener")
			<< "Compared to the information about files in zones from endpoint '"
			<< fromEndpointName << "' everything is up-to-date.";
		return Empty;
	}

	JsonRpcConnection::Ptr client;

	for (auto& conn : endpoint->GetClients()) {
		client = conn;
		break;
	}

	if (!client) {
		Log(LogNotice, "ApiListener")
			<< "Compared to the information about files in zones from endpoint '"
			<< fromEndpointName << "' not everything is up-to-date, but we're not connected.";
		return Empty;
	}

	Log(LogInformation, "ApiListener")
		<< "Compared to the information about files in zones from endpoint '" << fromEndpointName
		<< "' not everything is up-to-date. Requesting files of " << want->GetLength() << " zone(s).";

	client->SendMessage(new Dictionary({
		{ "jsonrpc", "2.0" },
		{ "method", "config::WantFiles" },
		{ "params", new Dictionary({
			{ "files", want }
		}) }
	}));

	return Empty;
}

Value ApiListener::ConfigWantFilesHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Log(LogNotice, "ApiListener")
		<< "Received request for files in zones: " << JsonEncode(params);

	/* check permissions */
	auto listener (ApiListener::GetInstance());

	if (!listener) {
		return Empty;
	}

	auto endpoint (origin->FromClient->GetEndpoint());
	auto identity (origin->FromClient->GetIdentity());
	auto clientZone (endpoint->GetZone());

	/* discard messages if the client is not configured on this node */
	if (!endpoint) {
		Log(LogNotice, "ApiListener")
			<< "Discarding 'config want files' message from '" << identity
			<< "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	auto zone (endpoint->GetZone());
	Dictionary::Ptr files (params->Get("files"));
	auto zonesDir (GetApiZonesDir());
	Dictionary::Ptr configUpdateV1 = new Dictionary();
	Dictionary::Ptr configUpdateV2 = new Dictionary();
	Dictionary::Ptr configUpdateChecksums = new Dictionary(); // new since 2.11
	ObjectLock oLock (files);

	for (auto& kv : files) {
		auto zone (Zone::GetByName(kv.first));

		if (!zone) {
			Log(LogWarning, "ApiListener")
				<< "No such zone '" << kv.first << "' in zones files request from '" << identity << "'.";
			continue;
		}

		if (!zone->IsChildOf(clientZone) && !zone->IsGlobal()) {
			Log(LogWarning, "ApiListener")
				<< "Unauthorized access to zone '" << kv.first << "' in zones files request from '" << identity << "'.";
			continue;
		}

		String zoneDir = zonesDir + kv.first;

		if (!Utility::PathExists(zoneDir)) {
			Log(LogWarning, "ApiListener")
				<< "No local config for zone '" << kv.first << "' for zones files request from '" << identity << "'.";
			continue;
		}

		Log(LogInformation, "ApiListener")
			<< "Syncing requested configuration files for " << (zone->IsGlobal() ? "global " : "")
			<< "zone '" << kv.first << "' to endpoint '" << endpoint->GetName() << "'.";

		auto config (LoadConfigDir(zoneDir));

		{
			auto wantPerZone (((Array::Ptr)kv.second)->ToSet<String>());

			for (auto& files : { config.UpdateV1, config.UpdateV2 }) {
				ObjectLock oLock (files);

				for (auto& kv : files) {
					if (!Utility::Match("/.*", kv.first) && wantPerZone.find(kv.first) == wantPerZone.end()) {
						// Not requested? Not included!
						files->Set(kv.first, Empty);
					}
				}
			}
		}

		configUpdateV1->Set(kv.first, config.UpdateV1);
		configUpdateV2->Set(kv.first, config.UpdateV2);
		configUpdateChecksums->Set(kv.first, config.Checksums); // new since 2.11
	}

	if (!(configUpdateV1->GetLength() + configUpdateV2->GetLength())) {
		Log(LogNotice, "ApiListener")
			<< "Not syncing any configuration files to endpoint '" << identity << "'.";
		return Empty;
	}

	for (auto& zone : ConfigType::GetObjectsByType<Zone>()) {
		auto zoneName (zone->GetName());

		if (!zone->IsChildOf(clientZone) && !zone->IsGlobal()) {
			continue;
		}

		if (!Utility::PathExists(zonesDir + zoneName)) {
			continue;
		}

		if (configUpdateV1->Contains(zoneName)) {
			continue;
		}

		// Not requested? Not included!
		configUpdateV1->Set(zoneName, Empty);
		configUpdateV2->Set(zoneName, Empty);
		configUpdateChecksums->Set(zoneName, Empty);
	}

	JsonRpcConnection::Ptr client;

	for (auto& conn : endpoint->GetClients()) {
		client = conn;
		break;
	}

	if (!client) {
		Log(LogNotice, "ApiListener")
			<< "Not syncing any configuration files to endpoint '" << identity << "' as we're not connected.";
		return Empty;
	}

	client->SendMessage(new Dictionary({
		{ "jsonrpc", "2.0" },
		{ "method", "config::Update" },
		{ "params", new Dictionary({
			{ "update", configUpdateV1 },
			{ "update_v2", configUpdateV2 },	// Since 2.4.2.
			{ "checksums", configUpdateChecksums } 	// Since 2.11.0.
		}) }
	}));

	return Empty;
}

Value ApiListener::ConfigWantZonesHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Log(LogNotice, "ApiListener")
		<< "Received request for zones: " << JsonEncode(params);

	/* check permissions */
	auto listener (ApiListener::GetInstance());

	if (!listener) {
		return Empty;
	}

	auto endpoint (origin->FromClient->GetEndpoint());
	auto identity (origin->FromClient->GetIdentity());
	auto clientZone (endpoint->GetZone());

	/* discard messages if the client is not configured on this node */
	if (!endpoint) {
		Log(LogNotice, "ApiListener")
			<< "Discarding 'config want zones' message from '" << identity
			<< "': Invalid endpoint origin (client not allowed).";
		return Empty;
	}

	auto zone (endpoint->GetZone());
	Array::Ptr zones (params->Get("zones"));
	auto zonesDir (GetApiZonesDir());
	Dictionary::Ptr haveFiles = new Dictionary();
	ObjectLock oLock (zones);

	for (auto& zoneName : zones) {
		auto zone (Zone::GetByName(zoneName));

		if (!zone) {
			Log(LogWarning, "ApiListener")
				<< "No such zone '" << zoneName << "' in zones request from '" << identity << "'.";
			continue;
		}

		if (!zone->IsChildOf(clientZone) && !zone->IsGlobal()) {
			Log(LogWarning, "ApiListener")
				<< "Unauthorized access to zone '" << zoneName << "' in zones request from '" << identity << "'.";
			continue;
		}

		String checksumsPath = zonesDir + zoneName + "/.checksums";
		std::ifstream fp (checksumsPath.CStr(), std::ifstream::binary);

		if (!fp) {
			Log(LogWarning, "ApiListener")
				<< "No local .checksums for zone '" << zoneName << "' for zones request from '" << identity << "'.";
			continue;
		}

		Log(LogNotice, "ApiListener")
			<< "Informing '" << identity << "' about files in zone '" << zoneName << "'.";

		haveFiles->Set(zoneName, JsonDecode(String(
			std::istreambuf_iterator<char>(fp), std::istreambuf_iterator<char>()
		)));
	}

	if (!haveFiles->GetLength()) {
		Log(LogNotice, "ApiListener")
			<< "Not informing '" << identity << "' about files in any zone.";
		return Empty;
	}

	JsonRpcConnection::Ptr client;

	for (auto& conn : endpoint->GetClients()) {
		client = conn;
		break;
	}

	if (!client) {
		Log(LogNotice, "ApiListener")
			<< "Not informing '" << identity << "' about files in any zone as we're not connected.";
		return Empty;
	}

	Log(LogInformation, "ApiListener")
		<< "Informing '" << identity << "' about files in " << haveFiles->GetLength() << " zone(s).";

	client->SendMessage(new Dictionary({
		{ "jsonrpc", "2.0" },
		{ "method", "config::HaveFiles" },
		{ "params", new Dictionary({
			{ "checksums", haveFiles }
		}) }
	}));

	return Empty;
}

void ApiListener::HandleConfigUpdate(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	/* Only one transaction is allowed, concurrent message handlers need to wait.
	 * This affects two parent endpoints sending the config in the same moment.
	 */
	auto lock (Shared<boost::mutex::scoped_lock>::Make(m_ConfigSyncStageLock));

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

		// Load the current production config details.
		ConfigDirInformation productionConfigInfo = LoadConfigDir(productionConfigZoneDir);

		// Merge the config information.
		ConfigDirInformation newConfigInfo;

		if (kv.second.GetType() == ValueEmpty) {
			newConfigInfo = productionConfigInfo;
		} else {
			newConfigInfo.UpdateV1 = kv.second;

			// Load metadata.
			if (updateV2)
				newConfigInfo.UpdateV2 = updateV2->Get(kv.first);

			// Load checksums. New since 2.11.
			if (checksums)
				newConfigInfo.Checksums = checksums->Get(kv.first);

			for (auto& dict : { newConfigInfo.UpdateV1, newConfigInfo.UpdateV2 }) {
				ObjectLock oLock (dict);

				for (auto& kv : dict) {
					if (kv.second.GetType() == ValueEmpty) { // Partial update. New since v2.13.
						auto file (productionConfigZoneDir + kv.first);

						Log(LogDebug, "ApiListener")
							<< "Loading local file due to partial config update from endpoint '"
							<< fromEndpointName << "' for zone '" << zoneName << "': '" << file << "'";

						std::ifstream fp (file.CStr(), std::ifstream::binary);

						if (!fp) {
							Log(LogWarning, "ApiListener")
								<< "No such local file in config update from endpoint '" << fromEndpointName
								<< "' for zone '" << zoneName << "': '" << file << "'";
							continue;
						}

						// Should be kept in sync with ConfigGlobHandler().
						dict->Set(kv.first, Utility::ValidateUTF8(String(
							std::istreambuf_iterator<char>(fp), std::istreambuf_iterator<char>()
						)));
					}
				}
			}
		}

		// Merge updateV1 and updateV2
		Dictionary::Ptr productionConfig = MergeConfigUpdate(productionConfigInfo);
		Dictionary::Ptr newConfig = MergeConfigUpdate(newConfigInfo);

		/* If we have received 'checksums' via cluster message, go for it.
		 * Otherwise do the old timestamp dance for versions < 2.11.
		 */
		if (checksums) {
			Log(LogInformation, "ApiListener")
				<< "Received configuration for zone '" << zoneName << "' from endpoint '"
				<< fromEndpointName << "'. Comparing the timestamp and checksums.";

			if (CompareTimestampsConfigChange(productionConfig, newConfig, stageConfigZoneDir)) {

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

			if (CompareTimestampsConfigChange(productionConfig, newConfig, stageConfigZoneDir)) {
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

		// If the update removes a path, delete it on disk and signal a config change.
		{
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
		AsyncTryActivateZonesStage(relativePaths, lock);
	} else {
		Log(LogInformation, "ApiListener")
			<< "Received configuration updates (" << count << ") from endpoint '" << fromEndpointName
			<< "' are equal to production, skipping validation and reload.";
	}
}

/**
 * Callback for stage config validation.
 * When validation was successful, the configuration is copied from
 * stage to production and a restart is triggered.
 * On failure, there's no restart and this is logged.
 *
 * @param pr Result of the validation process.
 * @param relativePaths Collected paths including the zone name, which are copied from stage to current directories.
 */
void ApiListener::TryActivateZonesStageCallback(const ProcessResult& pr,
	const std::vector<String>& relativePaths)
{
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
 * Spawns a new validation process and waits for its output.
 * Sets 'System.ZonesStageVarDir' to override the config validation zone dirs with our current stage.
 *
 * @param relativePaths Required for later file operations in the callback. Provides the zone name plus path in a list.
 */
void ApiListener::AsyncTryActivateZonesStage(const std::vector<String>& relativePaths, const Shared<boost::mutex::scoped_lock>::Ptr& lock)
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

	process->Run([relativePaths, lock](const ProcessResult& pr) {
		TryActivateZonesStageCallback(pr, relativePaths);
	});
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
ConfigDirInformation ApiListener::LoadConfigDir(const String& dir, const Dictionary::Ptr& contents)
{
	ConfigDirInformation config;
	config.UpdateV1 = new Dictionary();
	config.UpdateV2 = new Dictionary();
	config.Checksums = new Dictionary();

	Utility::GlobRecursive(dir, "*", std::bind(&ApiListener::ConfigGlobHandler, std::ref(config), std::ref(contents), dir, _1), GlobFile);

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
void ApiListener::ConfigGlobHandler(ConfigDirInformation& config, const Dictionary::Ptr& contents, const String& path, const String& file)
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

	if (contents) {
		contents->Set(relativePath, content);
	}
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
