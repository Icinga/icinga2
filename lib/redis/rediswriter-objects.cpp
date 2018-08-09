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
#include "icinga/command.hpp"
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

void RedisWriter::UpdateAllConfigObjects(void)
{
	AssertOnWorkQueue();

	double startTime = Utility::GetTime();

	std::map<Type*, std::vector<String>> deleteQueries;
	long long cursor = 0;

	std::map<String, String> lcTypes;
	for (const Type::Ptr& type : Type::GetAllTypes()) {
		lcTypes.emplace(type->GetName().ToLower(), type->GetName());
	}

	do {
		std::shared_ptr<redisReply> reply = ExecuteQuery({ "SCAN", Convert::ToString(cursor), "MATCH", m_PrefixConfigObject + "*", "COUNT", "1000" });

		VERIFY(reply->type == REDIS_REPLY_ARRAY);
		VERIFY(reply->elements % 2 == 0);

		redisReply *cursorReply = reply->element[0];
		cursor = Convert::ToLong(cursorReply->str);

		redisReply *keysReply = reply->element[1];

		for (size_t i = 0; i < keysReply->elements; i++) {
			redisReply *keyReply = keysReply->element[i];
			VERIFY(keyReply->type == REDIS_REPLY_STRING);

			String key = keyReply->str;
			String namePair = key.SubStr(m_PrefixConfigObject.GetLength());

			String::SizeType pos = namePair.FindFirstOf(":");

			if (pos == String::NPos)
				continue;

			String type = namePair.SubStr(0, pos);
			String name = namePair.SubStr(pos + 1);

			auto actualTypeName = lcTypes.find(type);

			if (actualTypeName == lcTypes.end())
				continue;

			Type::Ptr ptype = Type::GetByName(actualTypeName->second);
			auto& deleteQuery = deleteQueries[ptype.get()];

			if (deleteQuery.empty()) {
				deleteQuery.emplace_back("DEL");
				deleteQuery.emplace_back(m_PrefixConfigCheckSum + type);
			}

			deleteQuery.push_back(m_PrefixConfigObject + type + ":" + name);
			deleteQuery.push_back(m_PrefixStatusObject + type + ":" + name);
		}
	} while (cursor != 0);

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		ConfigType *ctype = dynamic_cast<ConfigType *>(type.get());

		if (!ctype)
			continue;

		ExecuteQuery({ "MULTI" });

		/* Delete obsolete object keys first. */
		auto& deleteQuery = deleteQueries[type.get()];

		if (deleteQuery.size() > 1)
			ExecuteQuery(deleteQuery);

		String typeName = type->GetName().ToLower();

		/* fetch all objects and dump them */
		for (const ConfigObject::Ptr& object : ctype->GetObjects()) {
			SendConfigUpdate(object, false);
			SendStatusUpdate(object, false);
		}

		/* publish config type dump finished */
		ExecuteQuery({ "PUBLISH", "icinga:config:dump", typeName });

		ExecuteQuery({ "EXEC" });
	}

	Log(LogInformation, "RedisWriter")
		<< "Initial config/status dump finished in " << Utility::GetTime() - startTime << " seconds.";
}

static ConfigObject::Ptr GetHostGroup(const String& name)
{
	return ConfigObject::GetObject<HostGroup>(name);
}

static ConfigObject::Ptr GetServiceGroup(const String& name)
{
	return ConfigObject::GetObject<ServiceGroup>(name);
}

static ConfigObject::Ptr GetUserGroup(const String& name)
{
	return ConfigObject::GetObject<UserGroup>(name);
}

static ConfigObject::Ptr GetClude(const String& name)
{
	return ConfigObject::GetObject<TimePeriod>(name);
}

void RedisWriter::SendConfigUpdate(const ConfigObject::Ptr& object, bool useTransaction, bool runtimeUpdate)
{
	AssertOnWorkQueue();

	/* during startup we might send duplicated object config, ignore them without any connection */
	if (!m_Context)
		return;

	/* TODO: This isn't essentially correct as we don't keep track of config objects ourselves. This would avoid duplicated config updates at startup.
	if (!runtimeUpdate && m_ConfigDumpInProgress)
		return;
	*/

	if (useTransaction)
		ExecuteQuery({ "MULTI" });

	/* Send all object attributes to Redis, no extra checksums involved here. */
	UpdateObjectAttrs(m_PrefixConfigObject, object, FAConfig);

	/* Calculate object specific checksums and store them in a different namespace. */
	Type::Ptr type = object->GetReflectionType();

	String typeName = type->GetName().ToLower();
	String objectKey = GetObjectIdentifier(object);

	std::set<String> propertiesBlacklist ({"name", "__name", "package", "source_location", "templates"});

	Dictionary::Ptr checkSums = new Dictionary();

	checkSums->Set("name_checksum", CalculateCheckSumString(object->GetShortName()));
	checkSums->Set("environment_checksum", CalculateCheckSumString(GetEnvironment()));

	auto endpoint (dynamic_pointer_cast<Endpoint>(object));

	if (endpoint) {
		auto endpointZone (endpoint->GetZone());

		if (endpointZone) {
			checkSums->Set("zone_checksum", GetObjectIdentifier(endpointZone));
		}
	} else {
		/* 'zone' is available for all config objects, therefore calculate the checksum. */
		auto zone (static_pointer_cast<Zone>(object->GetZone()));

		if (zone)
			checkSums->Set("zone_checksum", GetObjectIdentifier(zone));
	}

	User::Ptr user = dynamic_pointer_cast<User>(object);

	if (user) {
		propertiesBlacklist.emplace("groups");

		Array::Ptr groups;
		ConfigObject::Ptr (*getGroup)(const String& name);

		groups = user->GetGroups();
		getGroup = &::GetUserGroup;

		checkSums->Set("groups_checksum", CalculateCheckSumArray(groups));

		Array::Ptr groupChecksums = new Array();

		ObjectLock groupsLock (groups);
		ObjectLock groupChecksumsLock (groupChecksums);

		for (auto group : groups) {
			groupChecksums->Add(GetObjectIdentifier((*getGroup)(group.Get<String>())));
		}

		checkSums->Set("group_checksums", groupChecksums);

		auto period (user->GetPeriod());

		if (period)
			checkSums->Set("period_checksum", GetObjectIdentifier(period));
	}

	Notification::Ptr notification = dynamic_pointer_cast<Notification>(object);

	if (notification) {
		Host::Ptr host;
		Service::Ptr service;
		auto users (notification->GetUsers());
		Array::Ptr userChecksums = new Array();
		Array::Ptr userNames = new Array();
		auto usergroups (notification->GetUserGroups());
		Array::Ptr usergroupChecksums = new Array();
		Array::Ptr usergroupNames = new Array();

		tie(host, service) = GetHostService(notification->GetCheckable());

		checkSums->Set("host_checksum", GetObjectIdentifier(host));
		checkSums->Set("command_checksum", GetObjectIdentifier(notification->GetCommand()));

		if (service)
			checkSums->Set("service_checksum", GetObjectIdentifier(service));

		userChecksums->Reserve(users.size());
		userNames->Reserve(users.size());

		for (auto& user : users) {
			userChecksums->Add(GetObjectIdentifier(user));
			userNames->Add(user->GetName());
		}

		checkSums->Set("user_checksums", userChecksums);
		checkSums->Set("users_checksum", CalculateCheckSumArray(userNames));

		usergroupChecksums->Reserve(usergroups.size());
		usergroupNames->Reserve(usergroups.size());

		for (auto& usergroup : usergroups) {
			usergroupChecksums->Add(GetObjectIdentifier(usergroup));
			usergroupNames->Add(usergroup->GetName());
		}

		checkSums->Set("usergroup_checksums", usergroupChecksums);
		checkSums->Set("usergroups_checksum", CalculateCheckSumArray(usergroupNames));
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
			getGroup = &::GetServiceGroup;

			/* Calculate the host_checksum */
			checkSums->Set("host_checksum", GetObjectIdentifier(host));
		} else {
			groups = host->GetGroups();
			getGroup = &::GetHostGroup;
		}

		checkSums->Set("groups_checksum", CalculateCheckSumArray(groups));

		Array::Ptr groupChecksums = new Array();

		ObjectLock groupsLock (groups);
		ObjectLock groupChecksumsLock (groupChecksums);

		for (auto group : groups) {
			groupChecksums->Add(GetObjectIdentifier((*getGroup)(group.Get<String>())));
		}

		checkSums->Set("group_checksums", groupChecksums);

		/* command_endpoint_checksum / node_checksum */
		Endpoint::Ptr commandEndpoint = checkable->GetCommandEndpoint();

		if (commandEndpoint)
			checkSums->Set("command_endpoint_checksum", GetObjectIdentifier(commandEndpoint));

		/* *_command_checksum */
		checkSums->Set("check_command_checksum", GetObjectIdentifier(checkable->GetCheckCommand()));

		EventCommand::Ptr eventCommand = checkable->GetEventCommand();

		if (eventCommand)
			checkSums->Set("event_command_checksum", GetObjectIdentifier(eventCommand));

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
	} else {
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

			Zone::Ptr parentZone = zone->GetParent();

			if (parentZone)
				checkSums->Set("parent_checksum", GetObjectIdentifier(parentZone));

			Array::Ptr parents (new Array);

			for (auto& parent : zone->GetAllParentsRaw()) {
				parents->Add(GetObjectIdentifier(parent));
			}

			checkSums->Set("all_parents_checksums", parents);
			checkSums->Set("all_parents_checksum", HashValue(zone->GetAllParents()));
		} else {
			/* zone_checksum for endpoints already is calculated above. */

			auto command (dynamic_pointer_cast<Command>(object));

			if (command) {
				Dictionary::Ptr arguments = command->GetArguments();
				Dictionary::Ptr argumentChecksums = new Dictionary;

				if (arguments) {
					ObjectLock argumentsLock (arguments);

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
					ObjectLock argumentsLock (envvars);

					for (auto& kv : envvars) {
						envvarChecksums->Set(kv.first, HashValue(kv.second));
					}
				}

				checkSums->Set("envvars_checksum", HashValue(envvars));
				checkSums->Set("envvar_checksums", envvarChecksums);
				propertiesBlacklist.emplace("env");
			} else {
				auto timeperiod (dynamic_pointer_cast<TimePeriod>(object));

				if (timeperiod) {
					Dictionary::Ptr ranges = timeperiod->GetRanges();

					checkSums->Set("ranges_checksum", HashValue(ranges));
					propertiesBlacklist.emplace("ranges");

					// Compute checksums for Includes (like groups)
					Array::Ptr includes;
					ConfigObject::Ptr (*getInclude)(const String& name);

					includes = timeperiod->GetIncludes();
					getInclude = &::GetClude;

					checkSums->Set("includes_checksum", CalculateCheckSumArray(includes));

					Array::Ptr includeChecksums = new Array();

					ObjectLock includesLock (includes);
					ObjectLock includeChecksumsLock (includeChecksums);

					for (auto include : includes) {
						includeChecksums->Add(GetObjectIdentifier((*getInclude)(include.Get<String>())));
					}

					checkSums->Set("include_checksums", includeChecksums);

					// Compute checksums for Excludes (like groups)
					Array::Ptr excludes;
					ConfigObject::Ptr (*getExclude)(const String& name);

					excludes = timeperiod->GetExcludes();
					getExclude = &::GetClude;

					checkSums->Set("excludes_checksum", CalculateCheckSumArray(excludes));

					Array::Ptr excludeChecksums = new Array();

					ObjectLock excludesLock (excludes);
					ObjectLock excludeChecksumsLock (excludeChecksums);

					for (auto exclude : excludes) {
						excludeChecksums->Add(GetObjectIdentifier((*getExclude)(exclude.Get<String>())));
					}

					checkSums->Set("exclude_checksums", excludeChecksums);
				}
			}
		}
	}

	/* Custom var checksums. */
	CustomVarObject::Ptr customVarObject = dynamic_pointer_cast<CustomVarObject>(object);

	if (customVarObject) {
		propertiesBlacklist.emplace("vars");

		checkSums->Set("vars_checksum", CalculateCheckSumVars(customVarObject));

		auto vars (SerializeVars(customVarObject));

		if (vars) {
			auto varsJson (JsonEncode(vars));

			Log(LogDebug, "RedisWriter")
				<< "HSET " << m_PrefixConfigCustomVar + typeName << " " << objectKey << " " << varsJson;

			ExecuteQuery({ "HSET", m_PrefixConfigCustomVar + typeName, objectKey, varsJson });
		}
	}

	checkSums->Set("metadata_checksum", CalculateCheckSumMetadata(object));
	checkSums->Set("properties_checksum", CalculateCheckSumProperties(object, propertiesBlacklist));

	String checkSumsBody = JsonEncode(checkSums);

	Log(LogDebug, "RedisWriter")
		<< "HSET " << m_PrefixConfigCheckSum + typeName << " " << objectKey << " " << checkSumsBody;

	ExecuteQuery({ "HSET", m_PrefixConfigCheckSum + typeName, objectKey, checkSumsBody });


	/* Send an update event to subscribers. */
	if (runtimeUpdate) {
		ExecuteQuery({ "PUBLISH", "icinga:config:update", typeName + ":" + objectKey });
	}

	if (useTransaction)
		ExecuteQuery({ "EXEC" });
}

void RedisWriter::SendConfigDelete(const ConfigObject::Ptr& object)
{
	AssertOnWorkQueue();

	/* during startup we might send duplicated object config, ignore them without any connection */
	if (!m_Context)
		return;

	String typeName = object->GetReflectionType()->GetName().ToLower();
	String objectKey = GetObjectIdentifier(object);

	ExecuteQueries({
	    { "DEL", m_PrefixConfigObject + typeName + ":" + objectKey },
	    { "DEL", m_PrefixStatusObject + typeName + ":" + objectKey },
	    { "PUBLISH", "icinga:config:delete", typeName + ":" + objectKey }
	});

}

void RedisWriter::SendStatusUpdate(const ConfigObject::Ptr& object, bool useTransaction)
{
	AssertOnWorkQueue();

	/* during startup we might receive check results, ignore them without any connection */
	if (!m_Context)
		return;

	if (useTransaction)
		ExecuteQuery({ "MULTI" });

	UpdateObjectAttrs(m_PrefixStatusObject, object, FAState);

	if (useTransaction)
		ExecuteQuery({ "EXEC" });

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

void RedisWriter::UpdateObjectAttrs(const String& keyPrefix, const ConfigObject::Ptr& object, int fieldType)
{
	Type::Ptr type = object->GetReflectionType();

	String typeName = type->GetName().ToLower();

	/* Use the name checksum as unique key. */
	String objectKey = GetObjectIdentifier(object);

	std::vector<std::vector<String> > queries;

	queries.push_back({ "DEL", keyPrefix + typeName + ":" + objectKey });

	std::vector<String> hmsetCommand({ "HMSET", keyPrefix + typeName + ":" + objectKey });

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

		hmsetCommand.push_back(field.Name);

		Value sval = Serialize(val);
		hmsetCommand.push_back(JsonEncode(sval));
	}

	queries.push_back(hmsetCommand);

	ExecuteQueries(queries);
}

void RedisWriter::StateChangedHandler(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
		rw->m_WorkQueue.Enqueue(std::bind(&RedisWriter::SendStatusUpdate, rw, object, true));
	}
}

void RedisWriter::VersionChangedHandler(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	if (object->IsActive()) {
		/* Create or update the object config */
		for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
			rw->m_WorkQueue.Enqueue(std::bind(&RedisWriter::SendConfigUpdate, rw.get(), object, true, true));
		}
	} else if (!object->IsActive() && object->GetExtension("ConfigObjectDeleted")) { /* same as in apilistener-configsync.cpp */
		/* Delete object config */
		for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
			rw->m_WorkQueue.Enqueue(std::bind(&RedisWriter::SendConfigDelete, rw.get(), object));
		}
	}
}
