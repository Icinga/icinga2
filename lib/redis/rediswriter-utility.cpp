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

using namespace icinga;

String RedisWriter::FormatCheckSumBinary(const String& str)
{
	char output[20*2+1];
	for (int i = 0; i < 20; i++)
		sprintf(output + 2 * i, "%02x", str[i]);

	return output;
}

static Value l_DefaultEnv = "production";

String RedisWriter::GetIdentifier(const ConfigObject::Ptr& object)
{
	return HashValue((Array::Ptr)new Array({ScriptGlobal::Get("Environment", &l_DefaultEnv), object->GetName()}));
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

