/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icingadb/icingadb.hpp"
#include "base/configtype.hpp"
#include "base/object-packer.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/tlsutility.hpp"
#include "base/initialize.hpp"
#include "base/objectlock.hpp"
#include "base/array.hpp"
#include "base/scriptglobal.hpp"
#include "base/convert.hpp"
#include "base/json.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/host.hpp"
#include <boost/algorithm/string.hpp>
#include <cmath>
#include <map>
#include <utility>
#include <vector>

using namespace icinga;

String IcingaDB::FormatCheckSumBinary(const String& str)
{
	char output[20*2+1];
	for (int i = 0; i < 20; i++)
		sprintf(output + 2 * i, "%02x", str[i]);

	return output;
}

String IcingaDB::FormatCommandLine(const Value& commandLine)
{
	String result;
	if (commandLine.IsObjectType<Array>()) {
		Array::Ptr args = commandLine;
		bool first = true;

		ObjectLock olock(args);
		for (const Value& arg : args) {
			String token = "'" + Convert::ToString(arg) + "'";

			if (first)
				first = false;
			else
				result += String(1, ' ');

			result += token;
		}
	} else if (!commandLine.IsEmpty()) {
		result = commandLine;
		boost::algorithm::replace_all(result, "\'", "\\'");
		result = "'" + result + "'";
	}

	return result;
}

String IcingaDB::GetObjectIdentifier(const ConfigObject::Ptr& object)
{
	String identifier = object->GetIcingadbIdentifier();
	if (identifier.IsEmpty()) {
		identifier = HashValue(new Array({m_EnvironmentId, object->GetName()}));
		object->SetIcingadbIdentifier(identifier);
	}

	return identifier;
}

/**
 * Calculates a deterministic history event ID like SHA1(env, eventType, x...[, nt][, eventTime])
 *
 * Where SHA1(env, x...) = GetObjectIdentifier(object)
 */
String IcingaDB::CalcEventID(const char* eventType, const ConfigObject::Ptr& object, double eventTime, NotificationType nt)
{
	Array::Ptr rawId = new Array({object->GetName()});
	rawId->Insert(0, m_EnvironmentId);
	rawId->Insert(1, eventType);

	if (nt) {
		rawId->Add(GetNotificationTypeByEnum(nt));
	}

	if (eventTime) {
		rawId->Add(TimestampToMilliseconds(eventTime));
	}

	return HashValue(std::move(rawId));
}

static const std::set<String> metadataWhitelist ({"package", "source_location", "templates"});

/**
 * Prepare custom vars for being written to Redis
 *
 * object.vars = {
 *   "disks": {
 *     "disk": {},
 *     "disk /": {
 *       "disk_partitions": "/"
 *     }
 *   }
 * }
 *
 * return {
 *   SHA1(PackObject([
 *     EnvironmentId,
 *     "disks",
 *     {
 *       "disk": {},
 *       "disk /": {
 *         "disk_partitions": "/"
 *       }
 *     }
 *   ])): {
 *     "environment_id": EnvironmentId,
 *     "name_checksum": SHA1("disks"),
 *     "name": "disks",
 *     "value": {
 *       "disk": {},
 *       "disk /": {
 *         "disk_partitions": "/"
 *       }
 *     }
 *   }
 * }
 *
 * @param	Dictionary	Config object with custom vars
 *
 * @return 				JSON-like data structure for Redis
 */
Dictionary::Ptr IcingaDB::SerializeVars(const Dictionary::Ptr& vars)
{
	if (!vars)
		return nullptr;

	Dictionary::Ptr res = new Dictionary();

	ObjectLock olock(vars);

	for (auto& kv : vars) {
		res->Set(
			SHA1(PackObject((Array::Ptr)new Array({m_EnvironmentId, kv.first, kv.second}))),
			(Dictionary::Ptr)new Dictionary({
				{"environment_id", m_EnvironmentId},
				{"name_checksum", SHA1(kv.first)},
				{"name", kv.first},
				{"value", JsonEncode(kv.second)},
			})
		);
	}

	return res;
}

/**
 * Serialize a dependency edge state for Icinga DB
 *
 * @param dependencyGroup The state of the group the dependency is part of.
 * @param dep The dependency object to serialize.
 *
 * @return A dictionary with the serialized state.
 */
Dictionary::Ptr IcingaDB::SerializeDependencyEdgeState(const DependencyGroup::Ptr& dependencyGroup, const Dependency::Ptr& dep)
{
	String edgeStateId;
	// The edge state ID is computed a bit differently depending on whether this is for a redundancy group or not.
	// For redundancy groups, the state ID is supposed to represent the connection state between the redundancy group
	// and the parent Checkable of the given dependency. Hence, the outcome will always be different for each parent
	// Checkable of the redundancy group.
	if (dependencyGroup->IsRedundancyGroup()) {
		edgeStateId = HashValue(new Array{
			dependencyGroup->GetIcingaDBIdentifier(),
			GetObjectIdentifier(dep->GetParent()),
		});
	} else if (dependencyGroup->GetIcingaDBIdentifier().IsEmpty()) {
		// For non-redundant dependency groups, on the other hand, all dependency objects within that group will
		// always have the same parent Checkable. Likewise, the state ID will be always the same as well it doesn't
		// matter which dependency object is used to compute it. Therefore, it's sufficient to compute it only once
		// and all the other dependency objects can reuse the cached state ID.
		edgeStateId = HashValue(new Array{dependencyGroup->GetCompositeKey(), GetObjectIdentifier(dep->GetParent())});
		dependencyGroup->SetIcingaDBIdentifier(edgeStateId);
	} else {
		// Use the already computed state ID for the dependency group.
		edgeStateId = dependencyGroup->GetIcingaDBIdentifier();
	}

	return new Dictionary{
		{"id", std::move(edgeStateId)},
		{"environment_id", m_EnvironmentId},
		{"failed", !dep->IsAvailable(DependencyState) || !dep->GetParent()->IsReachable()}
	};
}

/**
 * Serialize the provided redundancy group state attributes.
 *
 * @param child The child checkable object to serialize the state for.
 * @param redundancyGroup The redundancy group object to serialize the state for.
 *
 * @return A dictionary with the serialized redundancy group state.
 */
Dictionary::Ptr IcingaDB::SerializeRedundancyGroupState(const Checkable::Ptr& child, const DependencyGroup::Ptr& redundancyGroup)
{
	auto state(redundancyGroup->GetState(child.get()));
	return new Dictionary{
		{"id", redundancyGroup->GetIcingaDBIdentifier()},
		{"environment_id", m_EnvironmentId},
		{"redundancy_group_id", redundancyGroup->GetIcingaDBIdentifier()},
		{"failed", state != DependencyGroup::State::Ok},
		{"is_reachable", state != DependencyGroup::State::Unreachable},
		{"last_state_change", TimestampToMilliseconds(Utility::GetTime())},
	};
}

const char* IcingaDB::GetNotificationTypeByEnum(NotificationType type)
{
	switch (type) {
		case NotificationDowntimeStart:
			return "downtime_start";
		case NotificationDowntimeEnd:
			return "downtime_end";
		case NotificationDowntimeRemoved:
			return "downtime_removed";
		case NotificationCustom:
			return "custom";
		case NotificationAcknowledgement:
			return "acknowledgement";
		case NotificationProblem:
			return "problem";
		case NotificationRecovery:
			return "recovery";
		case NotificationFlappingStart:
			return "flapping_start";
		case NotificationFlappingEnd:
			return "flapping_end";
	}

	VERIFY(!"Invalid notification type.");
}

static const std::set<String> propertiesBlacklistEmpty;

String IcingaDB::HashValue(const Value& value)
{
	return HashValue(value, propertiesBlacklistEmpty);
}

String IcingaDB::HashValue(const Value& value, const std::set<String>& propertiesBlacklist, bool propertiesWhitelist)
{
	Value temp;
	bool mutabl;

	Type::Ptr type = value.GetReflectionType();

	if (ConfigObject::TypeInstance->IsAssignableFrom(type)) {
		temp = Serialize(value, FAConfig);
		mutabl = true;
	} else {
		temp = value;
		mutabl = false;
	}

	if (propertiesBlacklist.size() && temp.IsObject()) {
		Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>((Object::Ptr)temp);

		if (dict) {
			if (!mutabl)
				dict = dict->ShallowClone();

			ObjectLock olock(dict);

			if (propertiesWhitelist) {
				auto current = dict->Begin();
				auto propertiesBlacklistEnd = propertiesBlacklist.end();

				while (current != dict->End()) {
					if (propertiesBlacklist.find(current->first) == propertiesBlacklistEnd) {
						dict->Remove(current++);
					} else {
						++current;
					}
				}
			} else {
				for (auto& property : propertiesBlacklist)
					dict->Remove(property);
			}

			if (!mutabl)
				temp = dict;
		}
	}

	return SHA1(PackObject(temp));
}

String IcingaDB::GetLowerCaseTypeNameDB(const ConfigObject::Ptr& obj)
{
	return obj->GetReflectionType()->GetName().ToLower();
}

long long IcingaDB::TimestampToMilliseconds(double timestamp) {
	// In addition to the limits of the Icinga DB MySQL (0 - 2^64) and PostgreSQL (0 - 2^63) schemata,
	// years not fitting in YYYY may cause problems, see e.g. https://github.com/golang/go/issues/4556.
	// RFC 3339: "All dates and times are assumed to be (...) somewhere between 0000AD and 9999AD."
	//
	// The below upper limit includes a safety buffer to make sure the timestamp is within 9999AD in all time zones:
	// $ date -ud @253402214400
	// Fri Dec 31 00:00:00 UTC 9999
	// $ TZ=Asia/Vladivostok date -d @253402214400
	// Fri Dec 31 10:00:00 +10 9999
	// $ TZ=America/Juneau date -d @253402214400
	// Thu Dec 30 15:00:00 AKST 9999
	return std::fmin(std::fmax(timestamp, 0.0), 253402214400.0) * 1000.0;
}

String IcingaDB::IcingaToStreamValue(const Value& value)
{
	switch (value.GetType()) {
		case ValueBoolean:
			return Convert::ToString(int(value));
		case ValueString:
			return Utility::ValidateUTF8(value);
		case ValueNumber:
		case ValueEmpty:
			return Convert::ToString(value);
		default:
			return JsonEncode(value);
	}
}

// Returns the items that exist in "arrayOld" but not in "arrayNew"
std::vector<Value> IcingaDB::GetArrayDeletedValues(const Array::Ptr& arrayOld, const Array::Ptr& arrayNew) {
	std::vector<Value> deletedValues;

	if (!arrayOld) {
		return deletedValues;
	}

	if (!arrayNew) {
		ObjectLock olock (arrayOld);
		return std::vector<Value>(arrayOld->Begin(), arrayOld->End());
	}

	std::vector<Value> vectorOld;
	{
		ObjectLock olock (arrayOld);
		vectorOld.assign(arrayOld->Begin(), arrayOld->End());
	}
	std::sort(vectorOld.begin(), vectorOld.end());
	vectorOld.erase(std::unique(vectorOld.begin(), vectorOld.end()), vectorOld.end());

	std::vector<Value> vectorNew;
	{
		ObjectLock olock (arrayNew);
		vectorNew.assign(arrayNew->Begin(), arrayNew->End());
	}
	std::sort(vectorNew.begin(), vectorNew.end());
	vectorNew.erase(std::unique(vectorNew.begin(), vectorNew.end()), vectorNew.end());

	std::set_difference(vectorOld.begin(), vectorOld.end(), vectorNew.begin(), vectorNew.end(), std::back_inserter(deletedValues));

	return deletedValues;
}

// Returns the keys that exist in "dictOld" but not in "dictNew"
std::vector<String> IcingaDB::GetDictionaryDeletedKeys(const Dictionary::Ptr& dictOld, const Dictionary::Ptr& dictNew) {
	std::vector<String> deletedKeys;

	if (!dictOld) {
		return deletedKeys;
	}

	std::vector<String> oldKeys = dictOld->GetKeys();

	if (!dictNew) {
		return oldKeys;
	}

	std::vector<String> newKeys = dictNew->GetKeys();

	std::set_difference(oldKeys.begin(), oldKeys.end(), newKeys.begin(), newKeys.end(), std::back_inserter(deletedKeys));

	return deletedKeys;
}
