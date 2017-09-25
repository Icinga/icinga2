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
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/serializer.hpp"
#include "base/tlsutility.hpp"
#include "base/initialize.hpp"

using namespace icinga;

String RedisWriter::FormatCheckSumBinary(const String& str)
{
	char output[20*2+1];
	for (int i = 0; i < 20; i++)
		sprintf(output + 2 * i, "%02x", str[i]);

	return output;
}

String RedisWriter::CalculateCheckSumString(const String& str, bool binary)
{
	return SHA1(str, binary);
}

String RedisWriter::CalculateCheckSumGroups(const Array::Ptr& groups, bool binary)
{
	String output;

	ObjectLock olock(groups);

	for (const String& group : groups) {
		output += SHA1(group, true); //binary checksum required here
	}

	return SHA1(output, binary);
}

String RedisWriter::CalculateCheckSumProperties(const ConfigObject::Ptr& object, bool binary)
{
	//TODO: consider precision of 6 for double values; use specific config fields for hashing?
	return HashValue(object, binary);
}

String RedisWriter::CalculateCheckSumVars(const ConfigObject::Ptr& object, bool binary)
{
	CustomVarObject::Ptr customVarObject = dynamic_pointer_cast<CustomVarObject>(object);

	if (!customVarObject)
		return HashValue(Empty, binary);

	Dictionary::Ptr vars = customVarObject->GetVars();

	if (!vars)
		return HashValue(Empty, binary);

	return HashValue(vars, binary);
}

String RedisWriter::HashValue(const Value& value, bool binary)
{
	Value temp;

	Type::Ptr type = value.GetReflectionType();

	if (ConfigObject::TypeInstance->IsAssignableFrom(type))
		temp = Serialize(value, FAConfig);
	else
		temp = value;

	return SHA1(JsonEncode(temp), binary);
}

Dictionary::Ptr RedisWriter::SerializeObjectAttrs(const Object::Ptr& object, int fieldType)
{
	Type::Ptr type = object->GetReflectionType();

	Dictionary::Ptr resultAttrs = new Dictionary();

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

		Value sval = Serialize(val);
		resultAttrs->Set(field.Name, sval);
	}

	return resultAttrs;
}

