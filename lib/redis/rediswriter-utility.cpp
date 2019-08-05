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

#include "redis/rediswriter.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/host.hpp"
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
#include <map>
#include <utility>
#include <vector>
#include <boost/algorithm/string.hpp>


using namespace icinga;

String RedisWriter::FormatCheckSumBinary(const String& str)
{
	char output[20*2+1];
	for (int i = 0; i < 20; i++)
		sprintf(output + 2 * i, "%02x", str[i]);

	return output;
}

String RedisWriter::FormatCommandLine(const Value& commandLine)
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

String RedisWriter::GetEnvironment()
{
	return ConfigType::GetObjectsByType<IcingaApplication>()[0]->GetEnvironment();
}

String RedisWriter::GetObjectIdentifier(const ConfigObject::Ptr& object)
{
	Type::Ptr type = object->GetReflectionType();

	if (type == CheckCommand::TypeInstance || type == NotificationCommand::TypeInstance || type == EventCommand::TypeInstance)
		return HashValue((Array::Ptr)new Array({GetEnvironment(), type->GetName(), object->GetName()}));
	else
		return HashValue((Array::Ptr)new Array({GetEnvironment(), object->GetName()}));
}

String RedisWriter::CalculateCheckSumString(const String& str)
{
	return SHA1(str);
}

String RedisWriter::CalculateCheckSumArray(const Array::Ptr& arr)
{
	/* Ensure that checksums happen in a defined order. */
	Array::Ptr tmpArr = arr->ShallowClone();

	tmpArr->Sort();

	return SHA1(PackObject(tmpArr));
}

String RedisWriter::CalculateCheckSumProperties(const ConfigObject::Ptr& object, const std::set<String>& propertiesBlacklist)
{
	//TODO: consider precision of 6 for double values; use specific config fields for hashing?
	return HashValue(object, propertiesBlacklist);
}

static const std::set<String> metadataWhitelist ({"package", "source_location", "templates"});

String RedisWriter::CalculateCheckSumMetadata(const ConfigObject::Ptr& object)
{
	return HashValue(object, metadataWhitelist, true);
}

String RedisWriter::CalculateCheckSumVars(const CustomVarObject::Ptr& object)
{
	Dictionary::Ptr vars = object->GetVars();

	if (!vars)
		return HashValue(Empty);

	return HashValue(vars);
}

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
Dictionary::Ptr RedisWriter::SerializeVars(const CustomVarObject::Ptr& object)
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
				{"env_id", envChecksum},
				{"name_checksum", SHA1(kv.first)},
				{"name", kv.first},
				{"value", JsonEncode(kv.second)},
			})
		);
	}

	return res;
}

static const std::set<String> propertiesBlacklistEmpty;

String RedisWriter::HashValue(const Value& value)
{
	return HashValue(value, propertiesBlacklistEmpty);
}

String RedisWriter::HashValue(const Value& value, const std::set<String>& propertiesBlacklist, bool propertiesWhitelist)
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

String RedisWriter::GetLowerCaseTypeNameDB(const ConfigObject::Ptr& obj)
{
	String typeName = obj->GetReflectionType()->GetName().ToLower();
	if (typeName == "downtime") {
		Downtime::Ptr downtime = dynamic_pointer_cast<Downtime>(obj);
		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(downtime->GetCheckable());
		if (service)
			typeName = "servicedowntime";
		else
			typeName = "hostdowntime";
	} else if (typeName == "comment") {
		Comment::Ptr comment = dynamic_pointer_cast<Comment>(obj);
		Host::Ptr host;
		Service::Ptr service;
		tie(host, service) = GetHostService(comment->GetCheckable());
		if (service)
			typeName = "servicecomment";
		else
			typeName = "hostcomment";
	}

	return typeName;
}
