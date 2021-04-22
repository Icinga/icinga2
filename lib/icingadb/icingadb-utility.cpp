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

String IcingaDB::GetEnvironment()
{
	return ConfigType::GetObjectsByType<IcingaApplication>()[0]->GetEnvironment();
}

ArrayData IcingaDB::GetObjectIdentifiersWithoutEnv(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	if (type == CheckCommand::TypeInstance || type == NotificationCommand::TypeInstance || type == EventCommand::TypeInstance)
		return {type->GetName(), object->GetName()};
	else
		return {object->GetName()};
}

String IcingaDB::GetObjectIdentifier(const ConfigObject::Ptr& object)
{
	return HashValue(new Array(Prepend(GetEnvironment(), GetObjectIdentifiersWithoutEnv(object))));
}

static const std::set<String> metadataWhitelist ({"package", "source_location", "templates"});

/**
 * Prepare object's custom vars for being written to Redis
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
 *     Environment,
 *     "disks",
 *     {
 *       "disk": {},
 *       "disk /": {
 *         "disk_partitions": "/"
 *       }
 *     }
 *   ])): {
 *     "envId": SHA1(Environment),
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
 * @param	object	Config object with custom vars
 *
 * @return 			JSON-like data structure for Redis
 */
Dictionary::Ptr IcingaDB::SerializeVars(const CustomVarObject::Ptr& object)
{
	Dictionary::Ptr vars = object->GetVars();

	if (!vars)
		return nullptr;

	Dictionary::Ptr res = new Dictionary();
	auto env (GetEnvironment());
	auto envChecksum (SHA1(env));

	ObjectLock olock(vars);

	for (auto& kv : vars) {
		res->Set(
			SHA1(PackObject((Array::Ptr)new Array({env, kv.first, kv.second}))),
			(Dictionary::Ptr)new Dictionary({
				{"environment_id", envChecksum},
				{"name_checksum", SHA1(kv.first)},
				{"name", kv.first},
				{"value", JsonEncode(kv.second)},
			})
		);
	}

	return res;
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
	return static_cast<long long>(timestamp * 1000);
}

String IcingaDB::IcingaToStreamValue(const Value& value)
{
	switch (value.GetType()) {
		case ValueNumber:
			return Convert::ToString(value);
		case ValueBoolean:
			return Convert::ToString((unsigned short)value);
		case ValueString:
			return Utility::ValidateUTF8(value);
		case ValueEmpty:
			return Convert::ToString(value);
		default:
			return JsonEncode(value);
	}
}
