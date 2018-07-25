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
#include "base/object-packer.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/tlsutility.hpp"
#include "base/initialize.hpp"
#include "base/objectlock.hpp"
#include "base/array.hpp"
#include "base/scriptglobal.hpp"
#include <map>
#include <utility>
#include <vector>

using namespace icinga;

String RedisWriter::FormatCheckSumBinary(const String& str)
{
	char output[20*2+1];
	for (int i = 0; i < 20; i++)
		sprintf(output + 2 * i, "%02x", str[i]);

	return output;
}

static Value l_DefaultEnv = "production";

String RedisWriter::GetEnvironment()
{
	return ScriptGlobal::Get("Environment", &l_DefaultEnv);
}

String RedisWriter::GetIdentifier(const ConfigObject::Ptr& object)
{
	return HashValue((Array::Ptr)new Array({GetEnvironment(), object->GetReflectionType()->GetName(), object->GetName()}));
}

String RedisWriter::CalculateCheckSumString(const String& str)
{
	return SHA1(str);
}

String RedisWriter::CalculateCheckSumGroups(const Array::Ptr& groups)
{
	/* Ensure that checksums happen in a defined order. */
	Array::Ptr tmpGroups = groups->ShallowClone();

	tmpGroups->Sort();

	return SHA1(PackObject(tmpGroups));
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
 * Collect the leaves of haystack in needles
 *
 * haystack = {
 *   "disks": {
 *     "disk": {},
 *     "disk /": {
 *       "disk_partitions": "/"
 *     }
 *   }
 * }
 *
 * path = []
 *
 * needles = {
 *   "disks": [
 *     [
 *       ["disks", "disk /", "disk_partitions"],
 *       "/"
 *     ]
 *   ]
 * }
 *
 * @param	haystack	A config object's custom vars as returned by {@link CustomVarObject#GetVars()}
 * @param	path 		Used for buffering only, shall be empty
 * @param 	needles 	The result, a mapping from the top-level custom var key to a list of leaves with full path and value
 */
static void CollectScalarVars(Value haystack, std::vector<Value>& path, std::map<String, std::vector<std::pair<Array::Ptr, Value>>>& needles)
{
	switch (haystack.GetType()) {
		case ValueObject:
		{
			const Object::Ptr& obj = haystack.Get<Object::Ptr>();

			Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(obj);

			if (dict) {
				ObjectLock olock(dict);

				for (auto& kv : dict) {
					path.emplace_back(kv.first);
					CollectScalarVars(kv.second, path, needles);
					path.pop_back();
				}

				break;
			}

			Array::Ptr arr = dynamic_pointer_cast<Array>(obj);

			if (arr) {
				double index = 0.0;

				ObjectLock xlock(arr);

				for (auto& v : arr) {
					path.emplace_back(index);
					CollectScalarVars(v, path, needles);
					path.pop_back();

					index += 1.0;
				}

				break;
			}
		}

			haystack = Empty;

		case ValueString:
		case ValueNumber:
		case ValueBoolean:
		case ValueEmpty:
			needles[path[0].Get<String>()].emplace_back(Array::FromVector(path), haystack);
			break;

		default:
			VERIFY(!"Invalid variant type.");
	}
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
 *     "env_checksum": SHA1(Environment),
 *     "name_checksum": SHA1("disks"),
 *     "name": "disks",
 *     "value": {
 *       "disk": {},
 *       "disk /": {
 *         "disk_partitions": "/"
 *       }
 *     },
 *     "flat": {
 *       SHA1(PackObject(["disks", "disk /", "disk_partitions"])): {
 *         "name": ["disks", "disk /", "disk_partitions"],
 *         "value": "/"
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

	std::map<String, std::vector<std::pair<Array::Ptr, Value>>> scalarVars;

	{
		std::vector<Value> pathBuf;
		CollectScalarVars(vars, pathBuf, scalarVars);
	}

	Dictionary::Ptr res = new Dictionary();
	auto env (GetEnvironment());
	auto envChecksum (SHA1(env));

	ObjectLock olock(vars);

	for (auto& kv : vars) {
		Dictionary::Ptr flatVars = new Dictionary();

		{
			auto it (scalarVars.find(kv.first));
			if (it != scalarVars.end()) {
				for (auto& scalarVar : it->second) {
					flatVars->Set(SHA1(PackObject(scalarVar.first)), (Dictionary::Ptr)new Dictionary({
						{"name", scalarVar.first},
						{"value", scalarVar.second}
					}));
				}
			}
		}

		res->Set(
			SHA1(PackObject((Array::Ptr)new Array({env, kv.first, kv.second}))),
			(Dictionary::Ptr)new Dictionary({
				{"env_checksum", envChecksum},
				{"name_checksum", SHA1(kv.first)},
				{"name", kv.first},
				{"value", kv.second},
				{"flat", flatVars}
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

