/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-base.h"
#include <cJSON.h>

using namespace icinga;

Value Empty;

/**
 * Checks whether the variant is empty.
 *
 * @returns true if the variant is empty, false otherwise.
 */
bool Value::IsEmpty(void) const
{
	return (m_Value.type() == typeid(boost::blank));
}

/**
 * Checks whether the variant is scalar (i.e. not an object and not empty).
 *
 * @returns true if the variant is scalar, false otherwise.
 */
bool Value::IsScalar(void) const
{
	return !IsEmpty() && !IsObject();
}

/**
 * Checks whether the variant is a non-null object.
 *
 * @returns true if the variant is a non-null object, false otherwise.
 */
bool Value::IsObject(void) const
{
	return !IsEmpty() && (m_Value.type() == typeid(Object::Ptr));
}

/**
 * Converts a JSON object into a variant.
 *
 * @param json The JSON object.
 */
Value Value::FromJson(cJSON *json)
{
	if (json->type == cJSON_Number)
		return json->valuedouble;
	else if (json->type == cJSON_String)
		return String(json->valuestring);
	else if (json->type == cJSON_True)
		return 1;
	else if (json->type == cJSON_False)
		return 0;
	else if (json->type == cJSON_Object)
		return Dictionary::FromJson(json);
	else if (json->type == cJSON_NULL)
		return Value();
	else
		throw_exception(invalid_argument("Unsupported JSON type."));
}

/**
 * Serializes a variant into a string.
 *
 * @returns A string representing this variant.
 */
String Value::Serialize(void) const
{
	cJSON *json = ToJson();

	char *jsonString;

	if (Application::IsDebugging())
		jsonString = cJSON_Print(json);
	else
		jsonString = cJSON_PrintUnformatted(json);

	cJSON_Delete(json);

	String result = jsonString;

	free(jsonString);

	return result;
}

/**
 * Serializes the variant.
 *
 * @returns A JSON object representing this variant.
 */
cJSON *Value::ToJson(void) const
{
	if (m_Value.type() == typeid(long)) {
		return cJSON_CreateNumber(boost::get<long>(m_Value));
	} else if (m_Value.type() == typeid(double)) {
		return cJSON_CreateNumber(boost::get<double>(m_Value));
	} else if (m_Value.type() == typeid(String)) {
		return cJSON_CreateString(boost::get<String>(m_Value).CStr());
	} else if (m_Value.type() == typeid(Object::Ptr)) {
		if (IsObjectType<Dictionary>()) {
			Dictionary::Ptr dictionary = *this;
			return dictionary->ToJson();
		} else {
			Logger::Write(LogDebug, "base", "Ignoring unknown object while converting variant to JSON.");
			return cJSON_CreateNull();
		}
	} else if (m_Value.type() == typeid(boost::blank)) {
		return cJSON_CreateNull();
	} else {
		throw_exception(runtime_error("Invalid variant type."));
	}
}

/**
 * Deserializes the string representation of a variant.
 *
 * @param jsonString A JSON string obtained from Value::Serialize
 * @returns The newly deserialized variant.
 */
Value Value::Deserialize(const String& jsonString)
{
	cJSON *json = cJSON_Parse(jsonString.CStr());

	if (!json)
		throw_exception(runtime_error("Invalid JSON String"));

	Value value = FromJson(json);
	cJSON_Delete(json);

	return value;
}
