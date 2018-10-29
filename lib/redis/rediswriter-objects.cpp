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

#include "redis/rediswriter.hpp"
#include "redis/redisconnection.hpp"
#include "icinga/command.hpp"
#include "base/configtype.hpp"
#include "base/configobject.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/host.hpp"
#include "icinga/service.hpp"
#include "icinga/hostgroup.hpp"
#include "icinga/servicegroup.hpp"
#include "icinga/usergroup.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/timeperiod.hpp"
#include "remote/zone.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/tlsutility.hpp"
#include "base/initialize.hpp"
#include "base/convert.hpp"
#include "base/array.hpp"
#include "base/exception.hpp"
#include <map>
#include <set>

using namespace icinga;

INITIALIZE_ONCE(&RedisWriter::ConfigStaticInitialize);

void RedisWriter::ConfigStaticInitialize()
{
	/* triggered in ProcessCheckResult(), requires UpdateNextCheck() to be called before */
	ConfigObject::OnStateChanged.connect(std::bind(&RedisWriter::StateChangedHandler, _1));

	/* triggered on create, update and delete objects */
	ConfigObject::OnActiveChanged.connect(std::bind(&RedisWriter::VersionChangedHandler, _1));
	ConfigObject::OnVersionChanged.connect(std::bind(&RedisWriter::VersionChangedHandler, _1));
}

void RedisWriter::UpdateAllConfigObjects()
{
	double startTime = Utility::GetTime();

	// Use a Workqueue to pack objects in parallel
	WorkQueue upq(25000, Configuration::Concurrency);
	upq.SetName("RedisWriter:ConfigDump");

	typedef std::pair<ConfigType *, String> TypePair;
	std::vector<TypePair> types;

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		ConfigType *ctype = dynamic_cast<ConfigType *>(type.get());
		if (!ctype)
			continue;

		String lcType(type->GetName().ToLower());

		if (lcType == "downtime") {
			types.emplace_back(ctype, "hostdowntime");
			types.emplace_back(ctype, "servicedowntime");
		} else if (lcType == "comment") {
			types.emplace_back(ctype, "hostcomment");
			types.emplace_back(ctype, "servicecomment");
		} else {
			types.emplace_back(ctype, lcType);
		}
	}

	upq.ParallelFor(types, [this](const TypePair& type)
	{
		String lcType = type.second;
		m_Rcon->ExecuteQuery(
				{"DEL", m_PrefixConfigCheckSum + lcType, m_PrefixConfigObject + lcType, m_PrefixStatusObject + lcType});
		size_t bulkCounter = 0;
		auto attributes = std::vector<String>({"HMSET", m_PrefixConfigObject + lcType});
		auto customVars = std::vector<String>({"HMSET", m_PrefixConfigCustomVar + lcType});
		auto checksums = std::vector<String>({"HMSET", m_PrefixConfigCheckSum + lcType});

		for (const ConfigObject::Ptr& object : type.first->GetObjects()) {
			CreateConfigUpdate(object, lcType, attributes, customVars, checksums, false);
			SendStatusUpdate(object);
			bulkCounter++;
			if (!bulkCounter % 100) {
				if (attributes.size() > 2) {
					m_Rcon->ExecuteQuery(attributes);
					attributes.erase(attributes.begin() + 2, attributes.end());
				}
				if (customVars.size() > 2) {
					m_Rcon->ExecuteQuery(customVars);
					customVars.erase(customVars.begin() + 2, customVars.end());
				}
				if (checksums.size() > 2) {
					m_Rcon->ExecuteQuery(checksums);
					checksums.erase(checksums.begin() + 2, checksums.end());
				}
			}
		}
		if (attributes.size() > 2)
			m_Rcon->ExecuteQuery(attributes);
		if (customVars.size() > 2)
			m_Rcon->ExecuteQuery(customVars);
		if (checksums.size() > 2)
			m_Rcon->ExecuteQuery(checksums);

		Log(LogNotice, "RedisWriter")
				<< "Dumped " << bulkCounter << " objects of type " << type.second;
	});

	upq.Join();

	if (upq.HasExceptions()) {
		for (auto exc : upq.GetExceptions()) {
			Log(LogCritical, "RedisWriter")
					<< "Exception during ConfigDump: " << exc;
		}
	}

	Log(LogInformation, "RedisWriter")
			<< "Initial config/status dump finished in " << Utility::GetTime() - startTime << " seconds.";
}

template<typename ConfigType>
static ConfigObject::Ptr GetObjectByName(const String& name)
{
	return ConfigObject::GetObject<ConfigType>(name);
}

// Used to update a single object, used for runtime updates
void RedisWriter::SendConfigUpdate(const ConfigObject::Ptr& object, bool runtimeUpdate)
{
	String typeName = GetLowerCaseTypeNameDB(object);

	auto attributes = std::vector<String>({"HMSET", m_PrefixConfigObject + typeName});
	auto customVars = std::vector<String>({"HMSET", m_PrefixConfigCustomVar + typeName});
	auto checksums = std::vector<String>({"HMSET", m_PrefixConfigCheckSum + typeName});

	CreateConfigUpdate(object, typeName, attributes, customVars, checksums, runtimeUpdate);

	m_Rcon->ExecuteQuery(attributes);
	m_Rcon->ExecuteQuery(customVars);
	m_Rcon->ExecuteQuery(checksums);
}

void RedisWriter::MakeTypeChecksums(const ConfigObject::Ptr& object, std::set<String>& propertiesBlacklist, Dictionary::Ptr& checkSums)
{
	Endpoint::Ptr endpoint = dynamic_pointer_cast<Endpoint>(object);

	if (endpoint) {
		auto endpointZone(endpoint->GetZone());

		if (endpointZone) {
			checkSums->Set("zone_checksum", GetObjectIdentifier(endpointZone));
		}

		return;
	} else {
		/* 'zone' is available for all config objects, therefore calculate the checksum. */
		auto zone(static_pointer_cast<Zone>(object->GetZone()));

		if (zone)
			checkSums->Set("zone_checksum", GetObjectIdentifier(zone));
	}

	User::Ptr user = dynamic_pointer_cast<User>(object);
	if (user) {
		propertiesBlacklist.emplace("groups");

		Array::Ptr groups;
		ConfigObject::Ptr (*getGroup)(const String& name);

		groups = user->GetGroups();
		getGroup = &::GetObjectByName<UserGroup>;

		checkSums->Set("groups_checksum", CalculateCheckSumArray(groups));

		Array::Ptr groupChecksums = new Array();

		ObjectLock groupsLock(groups);
		ObjectLock groupChecksumsLock(groupChecksums);

		for (auto group : groups) {
			groupChecksums->Add(GetObjectIdentifier((*getGroup)(group.Get<String>())));
		}

		checkSums->Set("group_checksums", groupChecksums);

		auto period(user->GetPeriod());

		if (period)
			checkSums->Set("period_checksum", GetObjectIdentifier(period));

		return;
	}

	Notification::Ptr notification = dynamic_pointer_cast<Notification>(object);
	if (notification) {
		Host::Ptr host;
		Service::Ptr service;
		auto users(notification->GetUsers());
		Array::Ptr userChecksums = new Array();
		Array::Ptr userNames = new Array();
		auto usergroups(notification->GetUserGroups());
		Array::Ptr usergroupChecksums = new Array();
		Array::Ptr usergroupNames = new Array();

		tie(host, service) = GetHostService(notification->GetCheckable());

		checkSums->Set("host_checksum", GetObjectIdentifier(host));
		checkSums->Set("command_checksum", GetObjectIdentifier(notification->GetCommand()));

		if (service)
			checkSums->Set("service_checksum", GetObjectIdentifier(service));

		propertiesBlacklist.emplace("users");

		userChecksums->Reserve(users.size());
		userNames->Reserve(users.size());

		for (auto& user : users) {
			userChecksums->Add(GetObjectIdentifier(user));
			userNames->Add(user->GetName());
		}

		checkSums->Set("user_checksums", userChecksums);
		checkSums->Set("users_checksum", CalculateCheckSumArray(userNames));

		propertiesBlacklist.emplace("user_groups");

		usergroupChecksums->Reserve(usergroups.size());
		usergroupNames->Reserve(usergroups.size());

		for (auto& usergroup : usergroups) {
			usergroupChecksums->Add(GetObjectIdentifier(usergroup));
			usergroupNames->Add(usergroup->GetName());
		}

		checkSums->Set("usergroup_checksums", usergroupChecksums);
		checkSums->Set("usergroups_checksum", CalculateCheckSumArray(usergroupNames));
		return;
	}

	/* Calculate checkable checksums. */
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);
	if (checkable) {
		/* groups_checksum, group_checksums */
		propertiesBlacklist.emplace("groups");

		Host::Ptr host;
		Service::Ptr service;

		tie(host, service) = GetHostService(checkable);

		Array::Ptr groups;
		ConfigObject::Ptr (*getGroup)(const String& name);

		if (service) {
			groups = service->GetGroups();
			getGroup = &::GetObjectByName<ServiceGroup>;

			/* Calculate the host_checksum */
			checkSums->Set("host_checksum", GetObjectIdentifier(host));
		} else {
			groups = host->GetGroups();
			getGroup = &::GetObjectByName<HostGroup>;
		}

		checkSums->Set("groups_checksum", CalculateCheckSumArray(groups));

		Array::Ptr groupChecksums = new Array();

		ObjectLock groupsLock(groups);
		ObjectLock groupChecksumsLock(groupChecksums);

		for (auto group : groups) {
			groupChecksums->Add(GetObjectIdentifier((*getGroup)(group.Get<String>())));
		}

		checkSums->Set("group_checksums", groupChecksums);

		/* command_endpoint_checksum / node_checksum */
		Endpoint::Ptr commandEndpoint = checkable->GetCommandEndpoint();

		if (commandEndpoint)
			checkSums->Set("command_endpoint_checksum", GetObjectIdentifier(commandEndpoint));

		/* *_command_checksum */
		checkSums->Set("checkcommand_checksum", GetObjectIdentifier(checkable->GetCheckCommand()));

		EventCommand::Ptr eventCommand = checkable->GetEventCommand();

		if (eventCommand)
			checkSums->Set("eventcommand_checksum", GetObjectIdentifier(eventCommand));

		/* *_url_checksum, icon_image_checksum */
		String actionUrl = checkable->GetActionUrl();
		String notesUrl = checkable->GetNotesUrl();
		String iconImage = checkable->GetIconImage();

		if (!actionUrl.IsEmpty())
			checkSums->Set("action_url_checksum", CalculateCheckSumString(actionUrl));
		if (!notesUrl.IsEmpty())
			checkSums->Set("notes_url_checksum", CalculateCheckSumString(notesUrl));
		if (!iconImage.IsEmpty())
			checkSums->Set("icon_image_checksum", CalculateCheckSumString(iconImage));

		return;
	}

	Zone::Ptr zone = dynamic_pointer_cast<Zone>(object);
	if (zone) {
		propertiesBlacklist.emplace("endpoints");

		auto endpointObjects = zone->GetEndpoints();
		Array::Ptr endpoints = new Array();
		endpoints->Resize(endpointObjects.size());

		Array::SizeType i = 0;
		for (auto& endpointObject : endpointObjects) {
			endpoints->Set(i++, endpointObject->GetName());
		}

		checkSums->Set("endpoints_checksum", CalculateCheckSumArray(endpoints));

		Array::Ptr parents(new Array);

		for (auto& parent : zone->GetAllParentsRaw()) {
			parents->Add(GetObjectIdentifier(parent));
		}

		checkSums->Set("all_parents_checksums", parents);
		checkSums->Set("all_parents_checksum", HashValue(zone->GetAllParents()));
		return;
	}

	/* zone_checksum for endpoints already is calculated above. */

	Command::Ptr command = dynamic_pointer_cast<Command>(object);
	if (command) {
		Dictionary::Ptr arguments = command->GetArguments();
		Dictionary::Ptr argumentChecksums = new Dictionary;

		if (arguments) {
			ObjectLock argumentsLock(arguments);

			for (auto& kv : arguments) {
				argumentChecksums->Set(kv.first, HashValue(kv.second));
			}
		}

		checkSums->Set("arguments_checksum", HashValue(arguments));
		checkSums->Set("argument_checksums", argumentChecksums);
		propertiesBlacklist.emplace("arguments");

		Dictionary::Ptr envvars = command->GetEnv();
		Dictionary::Ptr envvarChecksums = new Dictionary;

		if (envvars) {
			ObjectLock argumentsLock(envvars);

			for (auto& kv : envvars) {
				envvarChecksums->Set(kv.first, HashValue(kv.second));
			}
		}

		checkSums->Set("envvars_checksum", HashValue(envvars));
		checkSums->Set("envvar_checksums", envvarChecksums);
		propertiesBlacklist.emplace("env");

		return;
	}

	TimePeriod::Ptr timeperiod = dynamic_pointer_cast<TimePeriod>(object);
	if (timeperiod) {
		Dictionary::Ptr ranges = timeperiod->GetRanges();

		checkSums->Set("ranges_checksum", HashValue(ranges));
		propertiesBlacklist.emplace("ranges");

		// Compute checksums for Includes (like groups)
		Array::Ptr includes;
		ConfigObject::Ptr (*getInclude)(const String& name);

		includes = timeperiod->GetIncludes();
		getInclude = &::GetObjectByName<TimePeriod>;

		checkSums->Set("includes_checksum", CalculateCheckSumArray(includes));

		Array::Ptr includeChecksums = new Array();

		ObjectLock includesLock(includes);
		ObjectLock includeChecksumsLock(includeChecksums);

		for (auto include : includes) {
			includeChecksums->Add(GetObjectIdentifier((*getInclude)(include.Get<String>())));
		}

		checkSums->Set("include_checksums", includeChecksums);

		// Compute checksums for Excludes (like groups)
		Array::Ptr excludes;
		ConfigObject::Ptr (*getExclude)(const String& name);

		excludes = timeperiod->GetExcludes();
		getExclude = &::GetObjectByName<TimePeriod>;

		checkSums->Set("excludes_checksum", CalculateCheckSumArray(excludes));

		Array::Ptr excludeChecksums = new Array();

		ObjectLock excludesLock(excludes);
		ObjectLock excludeChecksumsLock(excludeChecksums);

		for (auto exclude : excludes) {
			excludeChecksums->Add(GetObjectIdentifier((*getExclude)(exclude.Get<String>())));
		}

		checkSums->Set("exclude_checksums", excludeChecksums);

		return;
	}

	Comment::Ptr comment = dynamic_pointer_cast<Comment>(object);
	if (comment) {
		propertiesBlacklist.emplace("name");
		propertiesBlacklist.emplace("host_name");

		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(comment->GetCheckable());
		if (service) {
			propertiesBlacklist.emplace("service_name");
			checkSums->Set("service_checksum", GetObjectIdentifier(service));
		} else
			checkSums->Set("host_checksum", GetObjectIdentifier(host));

		return;
	}

	Downtime::Ptr downtime = dynamic_pointer_cast<Downtime>(object);
	if (downtime) {
		propertiesBlacklist.emplace("name");
		propertiesBlacklist.emplace("host_name");
		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(downtime->GetCheckable());

		if (service) {
			propertiesBlacklist.emplace("service_name");
			checkSums->Set("service_checksum", GetObjectIdentifier(service));
		} else
			checkSums->Set("host_checksum", GetObjectIdentifier(host));

		return;
	}
}

/* Creates a config update with computed checksums etc.
 * Writes attributes, customVars and checksums into the respective supplied vectors. Adds two values to each vector
 * (if applicable), first the key then the value. To use in a Redis command the command (e.g. HSET) and the key (e.g.
 * icinga:config:object:downtime) need to be prepended. There is nothing to indicate success or failure.
 */
void RedisWriter::CreateConfigUpdate(const ConfigObject::Ptr& object, const String typeName, std::vector<String>& attributes,
									 std::vector<String>& customVars, std::vector<String>& checksums,
									 bool runtimeUpdate)
{
	/* TODO: This isn't essentially correct as we don't keep track of config objects ourselves. This would avoid duplicated config updates at startup.
	if (!runtimeUpdate && m_ConfigDumpInProgress)
		return;
	*/

	if (m_Rcon == nullptr)
		return;

	String objectKey = GetObjectIdentifier(object);

	std::set<String> propertiesBlacklist({"name", "__name", "package", "source_location", "templates"});

	Dictionary::Ptr checkSums = new Dictionary();

	checkSums->Set("name_checksum", CalculateCheckSumString(object->GetShortName()));
	checkSums->Set("environment_checksum", CalculateCheckSumString(GetEnvironment()));

	MakeTypeChecksums(object, propertiesBlacklist, checkSums);

	/* Send all object attributes to Redis, no extra checksums involved here. */
	auto tempAttrs = (UpdateObjectAttrs(m_PrefixConfigObject, object, FAConfig, typeName));
	attributes.insert(attributes.end(), std::begin(tempAttrs), std::end(tempAttrs));

	/* Custom var checksums. */
	CustomVarObject::Ptr customVarObject = dynamic_pointer_cast<CustomVarObject>(object);

	if (customVarObject) {
		propertiesBlacklist.emplace("vars");

		checkSums->Set("vars_checksum", CalculateCheckSumVars(customVarObject));

		auto vars(SerializeVars(customVarObject));

		if (vars) {
			auto varsJson(JsonEncode(vars));

			customVars.emplace_back(objectKey);
			customVars.emplace_back(varsJson);
		}
	}

	checkSums->Set("metadata_checksum", CalculateCheckSumMetadata(object));

	/* TODO: Problem: This does not account for `is_in_effect`, `trigger_time` of downtimes. */
	checkSums->Set("properties_checksum", CalculateCheckSumProperties(object, propertiesBlacklist));

	String checkSumsBody = JsonEncode(checkSums);

	checksums.emplace_back(objectKey);
	checksums.emplace_back(checkSumsBody);

	/* Send an update event to subscribers. */
	if (runtimeUpdate) {
		m_Rcon->ExecuteQuery({"PUBLISH", "icinga:config:update", typeName + ":" + objectKey});
	}
}

void RedisWriter::SendConfigDelete(const ConfigObject::Ptr& object)
{
	String typeName = object->GetReflectionType()->GetName().ToLower();
	String objectKey = GetObjectIdentifier(object);

	m_Rcon->ExecuteQueries({
								   {"HDEL",    m_PrefixConfigObject + typeName, objectKey},
								   {"DEL",     m_PrefixStatusObject + typeName + ":" + objectKey},
								   {"PUBLISH", "icinga:config:delete",          typeName + ":" + objectKey}
						   });
}

void RedisWriter::SendStatusUpdate(const ConfigObject::Ptr& object)
{
	//TODO: Manage type names
	//TODO: Figure out what we need when we implement the history and state sync
//	UpdateObjectAttrs(m_PrefixStatusObject, object, FAState, "");


//	/* Serialize config object attributes */
//	Dictionary::Ptr objectAttrs = SerializeObjectAttrs(object, FAState);
//
//	String jsonBody = JsonEncode(objectAttrs);
//
//	String objectName = object->GetName();
//
//	ExecuteQuery({ "HSET", "icinga:status:" + typeName, objectName, jsonBody });
//
//	/* Icinga DB part for Icinga Web 2 */
//	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);
//
//	if (checkable) {
//		Dictionary::Ptr attrs = new Dictionary();
//		String tableName;
//		String objectCheckSum = CalculateCheckSumString(objectName);
//
//		Host::Ptr host;
//		Service::Ptr service;
//
//		tie(host, service) = GetHostService(checkable);
//
//		if (service) {
//			tableName = "servicestate";
//			attrs->Set("service_checksum", objectCheckSum);
//			attrs->Set("host_checksum", CalculateCheckSumString(host->GetName()));
//		} else {
//			tableName = "hoststate";
//			attrs->Set("host_checksum", objectCheckSum);
//		}
//
//		attrs->Set("last_check", checkable->GetLastCheck());
//		attrs->Set("next_check", checkable->GetNextCheck());
//
//		attrs->Set("severity", checkable->GetSeverity());
//
///*
//        'host_checksum'    => null,
//        'command'          => null, // JSON, array
//        'execution_start'  => null,
//        'execution_end'    => null,
//        'schedule_start'   => null,
//        'schedule_end'     => null,
//        'exit_status'      => null,
//        'output'           => null,
//        'performance_data' => null, // JSON, array
//
//
//10.0.3.12:6379> keys icinga:hoststate.*
//1) "icinga:hoststate.~\xf5a\x91+\x03\x97\x99\xb5(\x16 CYm\xb1\xdf\x85\xa2\xcb"
//10.0.3.12:6379> get "icinga:hoststate.~\xf5a\x91+\x03\x97\x99\xb5(\x16 CYm\xb1\xdf\x85\xa2\xcb"
//"{\"command\":[\"\\/usr\\/lib\\/nagios\\/plugins\\/check_ping\",\"-H\",\"127.0.0.1\",\"-c\",\"5000,100%\",\"-w\",\"3000,80%\"],\"execution_start\":1492007581.7624,\"execution_end\":1492007585.7654,\"schedule_start\":1492007581.7609,\"schedule_end\":1492007585.7655,\"exit_status\":0,\"output\":\"PING OK - Packet loss = 0%, RTA = 0.08 ms\",\"performance_data\":[\"rta=0.076000ms;3000.000000;5000.000000;0.000000\",\"pl=0%;80;100;0\"]}"
//
//*/
//
//		CheckResult::Ptr cr = checkable->GetLastCheckResult();
//
//		if (cr) {
//			attrs->Set("command", JsonEncode(cr->GetCommand()));
//			attrs->Set("execution_start", cr->GetExecutionStart());
//			attrs->Set("execution_end", cr->GetExecutionEnd());
//			attrs->Set("schedule_start", cr->GetScheduleStart());
//			attrs->Set("schedule_end", cr->GetScheduleStart());
//			attrs->Set("exit_status", cr->GetExitStatus());
//			attrs->Set("output", cr->GetOutput());
//			attrs->Set("performance_data", JsonEncode(cr->GetPerformanceData()));
//		}
//
//		String jsonAttrs = JsonEncode(attrs);
//		String key = "icinga:" + tableName + "." + objectCheckSum;
//		ExecuteQuery({ "SET", key, jsonAttrs });
//
//		/* expire in check_interval * attempts + timeout + some more seconds */
//		double expireTime = checkable->GetCheckInterval() * checkable->GetMaxCheckAttempts() + 60;
//		ExecuteQuery({ "EXPIRE", key, String(expireTime) });
//	}
}

std::vector<String>
RedisWriter::UpdateObjectAttrs(const String& keyPrefix, const ConfigObject::Ptr& object, int fieldType,
							   const String& typeNameOverride)
{
	Type::Ptr type = object->GetReflectionType();
	Dictionary::Ptr attrs(new Dictionary);

	for (int fid = 0; fid < type->GetFieldCount(); fid++) {
		Field field = type->GetFieldInfo(fid);

		if ((field.Attributes & fieldType) == 0)
			continue;

		Value val = object->GetField(fid);

		/* hide attributes which shouldn't be user-visible */
		if (field.Attributes & FANoUserView)
			continue;

		/* hide internal navigation fields */
		if (field.Attributes & FANavigation && !(field.Attributes & (FAConfig | FAState)))
			continue;

		attrs->Set(field.Name, Serialize(val));
	}

       /* Downtimes require in_effect, which is not an attribute */
       Downtime::Ptr downtime = dynamic_pointer_cast<Downtime>(object);
       if (downtime) {
               attrs->Set("in_effect", Serialize(downtime->IsInEffect()));
               attrs->Set("trigger_time", Serialize(downtime->GetTriggerTime()));
       }


	/* Use the name checksum as unique key. */
	String typeName = type->GetName().ToLower();
	if (!typeNameOverride.IsEmpty())
		typeName = typeNameOverride.ToLower();

	return {GetObjectIdentifier(object), JsonEncode(attrs)};
	//m_Rcon->ExecuteQuery({"HSET", keyPrefix + typeName, GetObjectIdentifier(object), JsonEncode(attrs)});
}

void RedisWriter::StateChangedHandler(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
		rw->m_WorkQueue.Enqueue(std::bind(&RedisWriter::SendStatusUpdate, rw, object));
	}
}

void RedisWriter::VersionChangedHandler(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	if (object->IsActive()) {
		// Create or update the object config
		for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
//			rw->m_WorkQueue.Enqueue(std::bind(&RedisWriter::SendConfigUpdate, rw, object, true));
		}
	} else if (!object->IsActive() &&
			   object->GetExtension("ConfigObjectDeleted")) { // same as in apilistener-configsync.cpp
		// Delete object config
		for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
			rw->m_WorkQueue.Enqueue(std::bind(&RedisWriter::SendConfigDelete, rw, object));
		}
	}
}
