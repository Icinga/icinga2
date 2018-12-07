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
#include "icinga/compatutility.hpp"
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
#include "icinga/pluginutility.hpp"
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
	Checkable::OnStateChange.connect(std::bind(&RedisWriter::StateChangeHandler, _1));
	/* triggered when acknowledged host/service goes back to ok and when the acknowledgement gets deleted */
	Checkable::OnAcknowledgementCleared.connect(std::bind(&RedisWriter::StateChangeHandler, _1));

	/* triggered on create, update and delete objects */
	ConfigObject::OnActiveChanged.connect(std::bind(&RedisWriter::VersionChangedHandler, _1));
	ConfigObject::OnVersionChanged.connect(std::bind(&RedisWriter::VersionChangedHandler, _1));

	/* fixed downtime start */
	Downtime::OnDowntimeStarted.connect(std::bind(&RedisWriter::DowntimeChangedHandler, _1));
	/* flexible downtime start */
	Downtime::OnDowntimeTriggered.connect(std::bind(&RedisWriter::DowntimeChangedHandler, _1));
	/* fixed/flexible downtime end */
	Downtime::OnDowntimeRemoved.connect(std::bind(&RedisWriter::DowntimeChangedHandler, _1));
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
				{"DEL", m_PrefixConfigCheckSum + lcType, m_PrefixConfigObject + lcType, m_PrefixStateObject + lcType});
		size_t bulkCounter = 0;
		std::vector<String> attributes = {"HMSET", m_PrefixConfigObject + lcType};
		std::vector<String> customVars = {"HMSET", m_PrefixConfigCustomVar + lcType};
		std::vector<String> checksums =  {"HMSET", m_PrefixConfigCheckSum + lcType};
		std::vector<String> states =     {"HMSET", m_PrefixStateObject + lcType };
		bool dumpState = (lcType == "host" || lcType == "service");

		for (const ConfigObject::Ptr& object : type.first->GetObjects()) {
			if (lcType != GetLowerCaseTypeNameDB(object))
				continue;
			CreateConfigUpdate(object, lcType, attributes, customVars, checksums, false);

			// Write out inital state for checkables
			if (dumpState) {
				states.emplace_back(GetObjectIdentifier(object));
				states.emplace_back(JsonEncode(SerializeState(dynamic_pointer_cast<Checkable>(object))));
			}

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
				if (states.size() > 2) {
					m_Rcon->ExecuteQuery(states);
					states.erase(states.begin() + 2, states.end());
				}
			}
		}
		if (attributes.size() > 2)
			m_Rcon->ExecuteQuery(attributes);
		if (customVars.size() > 2)
			m_Rcon->ExecuteQuery(customVars);
		if (checksums.size() > 2)
			m_Rcon->ExecuteQuery(checksums);
		if (states.size() > 2)
			m_Rcon->ExecuteQuery(states);

		m_Rcon->ExecuteQuery({"PUBLISH", "icinga:config:dump", lcType});

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

void RedisWriter::UpdateState(const Checkable::Ptr& checkable) {
	Dictionary::Ptr stateAttrs = SerializeState(checkable);

	m_Rcon->ExecuteQuery({"HSET", m_PrefixStateObject + GetLowerCaseTypeNameDB(checkable), GetObjectIdentifier(checkable), JsonEncode(stateAttrs)});
}

template<typename ConfigType>
static ConfigObject::Ptr GetObjectByName(const String& name)
{
	return ConfigObject::GetObject<ConfigType>(name);
}

// Used to update a single object, used for runtime updates
void RedisWriter::SendConfigUpdate(const ConfigObject::Ptr& object, bool runtimeUpdate)
{
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	String typeName = GetLowerCaseTypeNameDB(object);

	std::vector<String> attribute = {"HSET", m_PrefixConfigObject + typeName};
	std::vector<String> customVar = {"HSET", m_PrefixConfigCustomVar + typeName};
	std::vector<String> checksum =  {"HSET", m_PrefixConfigCheckSum + typeName};
	std::vector<String> state =     {"HSET", m_PrefixStateObject + typeName};


	CreateConfigUpdate(object, typeName, attribute, customVar, checksum, runtimeUpdate);
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);
	if (checkable) {
		m_Rcon->ExecuteQuery({"HSET", m_PrefixStateObject + typeName,
							  GetObjectIdentifier(checkable), JsonEncode(SerializeState(checkable))});
	}

	m_Rcon->ExecuteQuery(attribute);
	m_Rcon->ExecuteQuery(checksum);
	if (customVar.size() > 2)
		m_Rcon->ExecuteQuery(customVar);
}

// Takes object and collects IcingaDB relevant attributes and computes checksums. Returns whether the object is relevant
// for IcingaDB.
bool RedisWriter::PrepareObject(const ConfigObject::Ptr& object, Dictionary::Ptr& attributes, Dictionary::Ptr& checksums)
{
	checksums->Set("name_checksum", CalculateCheckSumString(object->GetName()));
	checksums->Set("environment_id", CalculateCheckSumString(GetEnvironment()));
	attributes->Set("name", object->GetName());

	Zone::Ptr ObjectsZone = static_pointer_cast<Zone>(object->GetZone());
	if (ObjectsZone) {
		checksums->Set("zone_id", GetObjectIdentifier(ObjectsZone));
		attributes->Set("zone", ObjectsZone->GetName());
	}

	Type::Ptr type = object->GetReflectionType();

	if (type == Endpoint::TypeInstance) {
		checksums->Set("properties_checksum", HashValue(attributes));

		return true;
	}

	if (type == Zone::TypeInstance) {
		Zone::Ptr zone = static_pointer_cast<Zone>(object);

		attributes->Set("is_global", zone->GetGlobal());
		checksums->Set("properties_checksum", HashValue(attributes));

		Array::Ptr endpoints = new Array();
		endpoints->Resize(zone->GetEndpoints().size());

		Array::SizeType i = 0;
		for (auto& endpointObject : zone->GetEndpoints()) {
			endpoints->Set(i++, endpointObject->GetName());
		}

		Array::Ptr parents(new Array);

		for (auto& parent : zone->GetAllParentsRaw()) {
			parents->Add(GetObjectIdentifier(parent));
		}

		checksums->Set("parent_ids", parents);
		checksums->Set("parents_checksum", HashValue(zone->GetAllParents()));

		return true;
	}

	if (type == Host::TypeInstance || type == Service::TypeInstance) {
		Checkable::Ptr checkable = static_pointer_cast<Checkable>(object);

		attributes->Set("checkcommand", checkable->GetCheckCommand()->GetName());
		attributes->Set("max_check_attempts", checkable->GetMaxCheckAttempts());
		attributes->Set("check_timeout", checkable->GetCheckTimeout());
		attributes->Set("check_interval", checkable->GetCheckInterval());
		attributes->Set("check_retry_interval", checkable->GetRetryInterval());
		attributes->Set("active_checks_enabled", checkable->GetEnableActiveChecks());
		attributes->Set("passive_checks_enabled", checkable->GetEnablePassiveChecks());
		attributes->Set("event_handler_enabled", checkable->GetEnableEventHandler());
		attributes->Set("notifications_enabled", checkable->GetEnableNotifications());
		attributes->Set("flapping_enabled", checkable->GetEnableFlapping());
		attributes->Set("flapping_threshold_low", checkable->GetFlappingThresholdLow());
		attributes->Set("flapping_threshold_high", checkable->GetFlappingThresholdHigh());
		attributes->Set("perfdata_enabled", checkable->GetEnablePerfdata());
		attributes->Set("is_volatile", checkable->GetVolatile());
		attributes->Set("notes", checkable->GetNotes());
		attributes->Set("icon_image_alt", checkable->GetIconImageAlt());

		checksums->Set("checkcommand_id", GetObjectIdentifier(checkable->GetCheckCommand()));

		Endpoint::Ptr commandEndpoint = checkable->GetCommandEndpoint();
		if (commandEndpoint) {
			checksums->Set("command_endpoint_id", GetObjectIdentifier(commandEndpoint));
			attributes->Set("command_endpoint", commandEndpoint->GetName());
		}

		TimePeriod::Ptr timePeriod = checkable->GetCheckPeriod();
		if (timePeriod) {
			checksums->Set("check_period_id", GetObjectIdentifier(timePeriod));
			attributes->Set("check_period", timePeriod->GetName());
		}

		EventCommand::Ptr eventCommand = checkable->GetEventCommand();
		if (eventCommand) {
			checksums->Set("eventcommand_id", GetObjectIdentifier(eventCommand));
			attributes->Set("eventcommand", eventCommand->GetName());
		}

		String actionUrl = checkable->GetActionUrl();
		String notesUrl = checkable->GetNotesUrl();
		String iconImage = checkable->GetIconImage();
		if (!actionUrl.IsEmpty())
			checksums->Set("action_url_id", CalculateCheckSumArray(new Array({GetEnvironment(), actionUrl})));
		if (!notesUrl.IsEmpty())
			checksums->Set("notes_url_id", CalculateCheckSumArray(new Array({GetEnvironment(), notesUrl})));
		if (!iconImage.IsEmpty())
			checksums->Set("icon_image_id", CalculateCheckSumArray(new Array({GetEnvironment(), iconImage})));


		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(checkable);

		Array::Ptr groups;
		ConfigObject::Ptr (*getGroup)(const String& name);

		if (service) {
			checksums->Set("host_id", GetObjectIdentifier(service->GetHost()));
			attributes->Set("display_name", service->GetDisplayName());

			groups = service->GetGroups();
			getGroup = &::GetObjectByName<ServiceGroup>;
		} else {
			attributes->Set("display_name", host->GetDisplayName());
			attributes->Set("address", host->GetAddress());
			attributes->Set("address6", host->GetAddress6());

			groups = host->GetGroups();
			getGroup = &::GetObjectByName<HostGroup>;
		}

		checksums->Set("groups_checksum", CalculateCheckSumArray(groups));

		Array::Ptr groupChecksums = new Array();

		ObjectLock groupsLock(groups);
		ObjectLock groupChecksumsLock(groupChecksums);

		for (auto group : groups) {
			groupChecksums->Add(GetObjectIdentifier((*getGroup)(group.Get<String>())));
		}

		checksums->Set("group_ids", groupChecksums);

		checksums->Set("properties_checksum", HashValue(attributes));

		return true;
	}

	if (type == User::TypeInstance) {
		User::Ptr user = static_pointer_cast<User>(object);

		attributes->Set("display_name", user->GetDisplayName());
		attributes->Set("email", user->GetEmail());
		attributes->Set("pager", user->GetPager());
		attributes->Set("notifications_enabled", user->GetEnableNotifications());
		attributes->Set("states", user->GetStates());
		attributes->Set("types", user->GetTypes());

		Array::Ptr groups;
		ConfigObject::Ptr (*getGroup)(const String& name);

		groups = user->GetGroups();
		getGroup = &::GetObjectByName<UserGroup>;

		checksums->Set("groups_checksum", CalculateCheckSumArray(groups));

		Array::Ptr groupChecksums = new Array();

		ObjectLock groupsLock(groups);
		ObjectLock groupChecksumsLock(groupChecksums);

		for (auto group : groups) {
			groupChecksums->Add(GetObjectIdentifier((*getGroup)(group.Get<String>())));
		}

		checksums->Set("group_ids", groupChecksums);

		if (user->GetPeriod())
			checksums->Set("period_id", GetObjectIdentifier(user->GetPeriod()));

		checksums->Set("properties_checksum", HashValue(attributes));

		return true;
	}

	if (type == TimePeriod::TypeInstance) {
		TimePeriod::Ptr timeperiod = static_pointer_cast<TimePeriod>(object);

		attributes->Set("display_name", timeperiod->GetDisplayName());
		attributes->Set("prefer_includes", timeperiod->GetPreferIncludes());

		checksums->Set("properties_checksum", HashValue(attributes));
		Dictionary::Ptr ranges = timeperiod->GetRanges();

		attributes->Set("ranges", ranges);
		checksums->Set("ranges_checksum", HashValue(ranges));

		// Compute checksums for Includes (like groups)
		Array::Ptr includes;
		ConfigObject::Ptr (*getInclude)(const String& name);
		includes = timeperiod->GetIncludes();
		getInclude = &::GetObjectByName<TimePeriod>;

		checksums->Set("includes_checksum", CalculateCheckSumArray(includes));

		Array::Ptr includeChecksums = new Array();

		ObjectLock includesLock(includes);
		ObjectLock includeChecksumsLock(includeChecksums);

		for (auto include : includes) {
			includeChecksums->Add(GetObjectIdentifier((*getInclude)(include.Get<String>())));
		}

		checksums->Set("include_ids", includeChecksums);

		// Compute checksums for Excludes (like groups)
		Array::Ptr excludes;
		ConfigObject::Ptr (*getExclude)(const String& name);

		excludes = timeperiod->GetExcludes();
		getExclude = &::GetObjectByName<TimePeriod>;

		checksums->Set("excludes_checksum", CalculateCheckSumArray(excludes));

		Array::Ptr excludeChecksums = new Array();

		ObjectLock excludesLock(excludes);
		ObjectLock excludeChecksumsLock(excludeChecksums);

		for (auto exclude : excludes) {
			excludeChecksums->Add(GetObjectIdentifier((*getExclude)(exclude.Get<String>())));
		}

		checksums->Set("exclude_ids", excludeChecksums);

		return true;
	}

	if (type == Notification::TypeInstance) {
		Notification::Ptr notification = static_pointer_cast<Notification>(object);

		Host::Ptr host;
		Service::Ptr service;
		std::set<User::Ptr> users = notification->GetUsers();
		Array::Ptr userChecksums = new Array();
		Array::Ptr userNames = new Array();
		auto usergroups(notification->GetUserGroups());
		Array::Ptr usergroupChecksums = new Array();
		Array::Ptr usergroupNames = new Array();

		tie(host, service) = GetHostService(notification->GetCheckable());

		checksums->Set("properties_checksum", HashValue(attributes));
		checksums->Set("host_id", GetObjectIdentifier(host));
		checksums->Set("command_id", GetObjectIdentifier(notification->GetCommand()));

		TimePeriod::Ptr timeperiod = notification->GetPeriod();
		if (timeperiod)
			checksums->Set("period_id", GetObjectIdentifier(timeperiod));

		if (service)
			checksums->Set("service_id", GetObjectIdentifier(service));

		userChecksums->Reserve(users.size());
		userNames->Reserve(users.size());

		for (auto& user : users) {
			userChecksums->Add(GetObjectIdentifier(user));
			userNames->Add(user->GetName());
		}

		checksums->Set("user_ids", userChecksums);
		checksums->Set("users_checksum", CalculateCheckSumArray(userNames));

		usergroupChecksums->Reserve(usergroups.size());
		usergroupNames->Reserve(usergroups.size());

		for (auto& usergroup : usergroups) {
			usergroupChecksums->Add(GetObjectIdentifier(usergroup));
			usergroupNames->Add(usergroup->GetName());
		}

		checksums->Set("usergroup_ids", usergroupChecksums);
		checksums->Set("usergroups_checksum", CalculateCheckSumArray(usergroupNames));

		if (notification->GetTimes()) {
			attributes->Set("times_begin", notification->GetTimes()->Get("begin"));
			attributes->Set("times_end",notification->GetTimes()->Get("end"));
		}

		return true;
	}

	if (type == Comment::TypeInstance) {
		Comment::Ptr comment = static_pointer_cast<Comment>(object);

		attributes->Set("author", comment->GetAuthor());
		attributes->Set("text", comment->GetText());
		attributes->Set("entry_type", comment->GetEntryType());
		attributes->Set("is_persistent", comment->GetPersistent());
		attributes->Set("expire_time", comment->GetExpireTime());

		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(comment->GetCheckable());
		if (service) {
			checksums->Set("service_id", GetObjectIdentifier(service));
		} else
			checksums->Set("host_id", GetObjectIdentifier(host));

		checksums->Set("properties_checksum", HashValue(attributes));

		return true;
	}

	if (type == Downtime::TypeInstance) {
		Downtime::Ptr downtime = static_pointer_cast<Downtime>(object);

		attributes->Set("author", downtime->GetAuthor());
		attributes->Set("comment", downtime->GetComment());
		attributes->Set("entry_time", downtime->GetEntryTime());
		attributes->Set("scheduled_start_time", downtime->GetStartTime());
		attributes->Set("scheduled_end_time", downtime->GetEndTime());
		attributes->Set("duration", downtime->GetDuration());
		attributes->Set("is_fixed", downtime->GetFixed());
		attributes->Set("is_in_effect", downtime->IsInEffect());
		if (downtime->IsInEffect())
			attributes->Set("actual_start_time", downtime->GetTriggerTime());

		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(downtime->GetCheckable());

		if (service) {
			checksums->Set("service_id", GetObjectIdentifier(service));
		} else
			checksums->Set("host_id", GetObjectIdentifier(host));

		checksums->Set("properties_checksum", HashValue(attributes));
		return true;
	}

	if (type == UserGroup::TypeInstance) {
		UserGroup::Ptr userGroup = static_pointer_cast<UserGroup>(object);

		attributes->Set("display_name", userGroup->GetDisplayName());

		checksums->Set("properties_checksum", HashValue(attributes));

		return true;
	}

	if (type == HostGroup::TypeInstance) {
		HostGroup::Ptr hostGroup = static_pointer_cast<HostGroup>(object);

		attributes->Set("display_name", hostGroup->GetDisplayName());

		checksums->Set("properties_checksum", HashValue(attributes));

		return true;
	}

	if (type == ServiceGroup::TypeInstance) {
		ServiceGroup::Ptr serviceGroup = static_pointer_cast<ServiceGroup>(object);

		attributes->Set("display_name", serviceGroup->GetDisplayName());

		checksums->Set("properties_checksum", HashValue(attributes));

		return true;
	}

	if (type == CheckCommand::TypeInstance || type == NotificationCommand::TypeInstance || type == EventCommand::TypeInstance) {
		Command::Ptr command = static_pointer_cast<Command>(object);

		if (dynamic_pointer_cast<CheckCommand>(object))
			attributes->Set("type", "CheckCommand");
		else if (dynamic_pointer_cast<EventCommand>(object))
			attributes->Set("type", "EventCommand");
		else
			attributes->Set("type", "NotificationCommand");

		attributes->Set("command", command->GetCommandLine());
		attributes->Set("timeout", command->GetTimeout());

		checksums->Set("properties_checksum", HashValue(attributes));

		Dictionary::Ptr arguments = command->GetArguments();
		Dictionary::Ptr argumentChecksums = new Dictionary;

		if (arguments) {
			ObjectLock argumentsLock(arguments);

			for (auto& kv : arguments) {
				argumentChecksums->Set(kv.first, HashValue(kv.second));
			}

			attributes->Set("arguments", arguments);
		}

		checksums->Set("arguments_checksum", HashValue(arguments));
		checksums->Set("argument_ids", argumentChecksums);

		Dictionary::Ptr envvars = command->GetEnv();
		Dictionary::Ptr envvarChecksums = new Dictionary;

		if (envvars) {
			ObjectLock argumentsLock(envvars);

			for (auto& kv : envvars) {
				envvarChecksums->Set(kv.first, HashValue(kv.second));
			}

			attributes->Set("envvars", envvars);
		}

		checksums->Set("envvars_checksum", HashValue(envvars));
		checksums->Set("envvar_ids", envvarChecksums);

		return true;
	}

	return false;
}

/* Creates a config update with computed checksums etc.
 * Writes attributes, customVars and checksums into the respective supplied vectors. Adds two values to each vector
 * (if applicable), first the key then the value. To use in a Redis command the command (e.g. HSET) and the key (e.g.
 * icinga:config:object:downtime) need to be prepended. There is nothing to indicate success or failure.
 */
void
RedisWriter::CreateConfigUpdate(const ConfigObject::Ptr& object, const String typeName, std::vector<String>& attributes,
								std::vector<String>& customVars, std::vector<String>& checksums,
								bool runtimeUpdate)
{
	/* TODO: This isn't essentially correct as we don't keep track of config objects ourselves. This would avoid duplicated config updates at startup.
	if (!runtimeUpdate && m_ConfigDumpInProgress)
		return;
	*/

	if (m_Rcon == nullptr)
		return;

	Dictionary::Ptr attr = new Dictionary;
	Dictionary::Ptr chksm = new Dictionary;

	if (!PrepareObject(object, attr, chksm))
		return;

	String objectKey = GetObjectIdentifier(object);

	attributes.emplace_back(objectKey);
	attributes.emplace_back(JsonEncode(attr));

	CustomVarObject::Ptr customVarObject = dynamic_pointer_cast<CustomVarObject>(object);

	if (customVarObject) {
		chksm->Set("customvars_checksum", CalculateCheckSumVars(customVarObject));

		auto vars(SerializeVars(customVarObject));

		if (vars) {
			auto varsJson(JsonEncode(vars));

			customVars.emplace_back(objectKey);
			customVars.emplace_back(varsJson);
		}
	}

	checksums.emplace_back(objectKey);
	checksums.emplace_back(JsonEncode(chksm));

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
								   {"DEL",     m_PrefixStateObject + typeName + ":" + objectKey},
								   {"PUBLISH", "icinga:config:delete", typeName + ":" + objectKey}
						   });
}

void RedisWriter::SendStatusUpdate(const ConfigObject::Ptr& object)
{
	Checkable::Ptr checkable = dynamic_pointer_cast<Checkable>(object);
	if (!checkable)
		return;

	Host::Ptr host;
	Service::Ptr service;

	tie(host, service) = GetHostService(checkable);

	String streamname;
	if (service)
		streamname = "icinga:state:stream:service";
	else
		streamname = "icinga:state:stream:host";

	Dictionary::Ptr objectAttrs = SerializeState(checkable);

	std::vector<String> streamadd({"XADD", streamname, "*"});
	ObjectLock olock(objectAttrs);
	for (const Dictionary::Pair& kv : objectAttrs) {
		streamadd.emplace_back(kv.first);
		streamadd.emplace_back(kv.second);
	}

	m_Rcon->ExecuteQuery(streamadd);
}

Dictionary::Ptr RedisWriter::SerializeState(const Checkable::Ptr& checkable)
{
	Dictionary::Ptr attrs = new Dictionary();

	Host::Ptr host;
	Service::Ptr service;

	tie(host, service) = GetHostService(checkable);

	attrs->Set("id", GetObjectIdentifier(checkable));;
	attrs->Set("env_id", CalculateCheckSumString(GetEnvironment()));
	attrs->Set("state_type", checkable->GetStateType());

	// TODO: last_hard/soft_state should be "previous".
	if (service) {
		attrs->Set("state", service->GetState());
		attrs->Set("last_soft_state", service->GetState());
		attrs->Set("last_hard_state", service->GetLastHardState());
	} else {
		attrs->Set("state", host->GetState());
		attrs->Set("last_soft_state", host->GetState());
		attrs->Set("last_hard_state", host->GetLastHardState());
	}

	attrs->Set("check_attempt", checkable->GetCheckAttempt());

	//attrs->Set("severity")
	//attrs->Set(checkable->GetSeverity());
	
	CheckResult::Ptr cr = checkable->GetLastCheckResult();

	if (cr) {
		String rawOutput = cr->GetOutput();
		if (!rawOutput.IsEmpty()) {
			size_t lineBreak = rawOutput.Find("\n");
			String output = rawOutput.SubStr(0, lineBreak);
			if (!output.IsEmpty())
				attrs->Set("output", rawOutput.SubStr(0, lineBreak));

			if (lineBreak > 0 && lineBreak != String::NPos) {
				String longOutput = rawOutput.SubStr(lineBreak+1, rawOutput.GetLength());
				if (!longOutput.IsEmpty())
					attrs->Set("long_output", longOutput);
			}
		}

		String perfData = PluginUtility::FormatPerfdata(cr->GetPerformanceData());
		if (!perfData.IsEmpty())
			attrs->Set("performance_data", perfData);

		if (!cr->GetCommand().IsEmpty())
			attrs->Set("commandline", FormatCommandLine(cr->GetCommand()));
		attrs->Set("execution_time", cr->CalculateExecutionTime());
		attrs->Set("latency", cr->CalculateLatency());
	}

	bool isProblem = !checkable->IsStateOK(checkable->GetStateRaw());
	attrs->Set("is_problem", isProblem);
	attrs->Set("is_handled", isProblem && (checkable->IsInDowntime() || checkable->IsAcknowledged()));
	attrs->Set("is_reachable", checkable->IsReachable());
	attrs->Set("is_flapping", checkable->IsFlapping());

	attrs->Set("is_acknowledged", checkable->IsAcknowledged());
	if (checkable->IsAcknowledged()) {
		Timestamp entry = 0;
		Comment::Ptr AckComment;
		for (const Comment::Ptr& c : checkable->GetComments()) {
			if (c->GetEntryType() == CommentAcknowledgement) {
				if (c->GetEntryTime() > entry) {
					entry = c->GetEntryTime();
					AckComment = c;
				}
			}
		}
		if (AckComment != nullptr) {
			attrs->Set("acknowledgement_comment_id", GetObjectIdentifier(AckComment));
		}
	}

	attrs->Set("in_downtime", checkable->IsInDowntime());

	if (checkable->GetCheckTimeout().IsEmpty())
		attrs->Set("check_timeout",checkable->GetCheckCommand()->GetTimeout());
	else
		attrs->Set("check_timeout", checkable->GetCheckTimeout());

	attrs->Set("last_update", Utility::GetTime());
	attrs->Set("last_state_change", checkable->GetLastStateChange());
	attrs->Set("next_check", checkable->GetNextCheck());

	return attrs;
}

std::vector<String>
RedisWriter::UpdateObjectAttrs(const ConfigObject::Ptr& object, int fieldType,
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

void RedisWriter::StateChangeHandler(const ConfigObject::Ptr &object)
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
			if (rw)
				rw->m_WorkQueue.Enqueue(std::bind(&RedisWriter::SendConfigUpdate, rw, object, true));
		}
	} else if (!object->IsActive() &&
			   object->GetExtension("ConfigObjectDeleted")) { // same as in apilistener-configsync.cpp
		// Delete object config
		for (const RedisWriter::Ptr& rw : ConfigType::GetObjectsByType<RedisWriter>()) {
			if (rw)
				rw->m_WorkQueue.Enqueue(std::bind(&RedisWriter::SendConfigDelete, rw, object));
		}
	}
}

void RedisWriter::DowntimeChangedHandler(const Downtime::Ptr& downtime)
{
	StateChangeHandler(downtime->GetCheckable());
}
