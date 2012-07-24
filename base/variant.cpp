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

/**
 * Checks whether the variant is empty.
 *
 * @returns true if the variant is empty, false otherwise.
 */
bool Variant::IsEmpty(void) const
{
	return (m_Value.type() == typeid(boost::blank));
}

bool Variant::IsScalar(void) const
{
	return !IsEmpty() && !IsObject();
}

bool Variant::IsObject(void) const
{
	return !IsEmpty() && (m_Value.type() == typeid(Object::Ptr));
}

Variant Variant::FromJson(cJSON *json)
{
	if (json->type == cJSON_Number)
		return json->valuedouble;
	else if (json->type == cJSON_String)
		return json->valuestring;
	else if (json->type == cJSON_True)
		return 1;
	else if (json->type == cJSON_False)
		return 0;
	else if (json->type == cJSON_Object)
		return Dictionary::FromJson(json);
	else if (json->type == cJSON_NULL)
		return Variant();
	else
		throw_exception(invalid_argument("Unsupported JSON type."));
}

string Variant::Serialize(void) const
{
	cJSON *json = ToJson();

	char *jsonString;

	if (!Application::GetInstance()->IsDebugging())
		jsonString = cJSON_Print(json);
	else
		jsonString = cJSON_PrintUnformatted(json);

	cJSON_Delete(json);

	string result = jsonString;

	free(jsonString);

	return result;
}

cJSON *Variant::ToJson(void) const
{
	if (m_Value.type() == typeid(long)) {
		return cJSON_CreateNumber(boost::get<long>(m_Value));
	} else if (m_Value.type() == typeid(double)) {
		return cJSON_CreateNumber(boost::get<double>(m_Value));
	} else if (m_Value.type() == typeid(string)) {
		return cJSON_CreateString(boost::get<string>(m_Value).c_str());
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

Variant Variant::Deserialize(const string& jsonString)
{
	cJSON *json = cJSON_Parse(jsonString.c_str());

	if (!json)
		throw_exception(runtime_error("Invalid JSON string"));

	Variant value = FromJson(json);
	cJSON_Delete(json);

	return value;
}
