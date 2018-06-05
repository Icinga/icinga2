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

using namespace icinga;

String RedisWriter::FormatCheckSumBinary(const String& str)
{
	char output[20*2+1];
	for (int i = 0; i < 20; i++)
		sprintf(output + 2 * i, "%02x", str[i]);

	return output;
}

String RedisWriter::CalculateCheckSumString(const String& str)
{
	return SHA1(str);
}

String RedisWriter::CalculateCheckSumGroups(const Array::Ptr& groups)
{
	String output;

	/* Ensure that checksums happen in a defined order. */
	Array::Ptr tmpGroups = groups->ShallowClone();

	tmpGroups->Sort();

	{
		ObjectLock olock(tmpGroups);

		for (const String& group : tmpGroups) {
			output += SHA1(group);
		}
	}

	return SHA1(output);
}

String RedisWriter::CalculateCheckSumProperties(const ConfigObject::Ptr& object)
{
	//TODO: consider precision of 6 for double values; use specific config fields for hashing?
	return HashValue(object);
}

String RedisWriter::CalculateCheckSumVars(const CustomVarObject::Ptr& object)
{
	Dictionary::Ptr vars = object->GetVars();

	if (!vars)
		return HashValue(Empty);

	return HashValue(vars);
}

String RedisWriter::HashValue(const Value& value)
{
	Value temp;

	Type::Ptr type = value.GetReflectionType();

	if (ConfigObject::TypeInstance->IsAssignableFrom(type))
		temp = Serialize(value, FAConfig);
	else
		temp = value;

	return SHA1(PackObject(temp));
}

