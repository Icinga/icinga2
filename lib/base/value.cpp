/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/application.h"
#include "base/array.h"
#include "base/logger_fwd.h"
#include "base/utility.h"
#include <cJSON.h>
#include <boost/lexical_cast.hpp>

using namespace icinga;

Value Empty;

Value::Value(void)
    : m_Value()
{ }

Value::Value(int value)
    : m_Value(double(value))
{ }

Value::Value(unsigned int value)
    : m_Value(double(value))
{ }

Value::Value(long value)
    : m_Value(double(value))
{ }

Value::Value(unsigned long value)
    : m_Value(double(value))
{ }

Value::Value(double value)
    : m_Value(value)
{ }

Value::Value(const String& value)
    : m_Value(value)
{ }

Value::Value(const char *value)
    : m_Value(String(value))
{ }

/**
 * Checks whether the variant is empty.
 *
 * @returns true if the variant is empty, false otherwise.
 */
bool Value::IsEmpty(void) const
{
	return (GetType() == ValueEmpty);
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
	return !IsEmpty() && (GetType() == ValueObject);
}

Value::operator double(void) const
{
	const double *value = boost::get<double>(&m_Value);

	if (value)
		return *value;

	if (IsEmpty())
		return 0;

	return boost::lexical_cast<double>(m_Value);
}

Value::operator String(void) const
{
	Object *object;
	double integral, fractional;

	switch (GetType()) {
		case ValueEmpty:
			return String();
		case ValueNumber:
			fractional = modf(boost::get<double>(m_Value), &integral);

			if (fractional != 0)
				return boost::lexical_cast<String>(m_Value);
			else
				return boost::lexical_cast<String>((long)integral);
		case ValueString:
			return boost::get<String>(m_Value);
		case ValueObject:
			object = boost::get<Object::Ptr>(m_Value).get();
			return "Object of type '" + Utility::GetTypeName(typeid(*object)) + "'";
		default:
			BOOST_THROW_EXCEPTION(std::runtime_error("Unknown value type."));
	}
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
	else if (json->type == cJSON_Array)
		return Array::FromJson(json);
	else if (json->type == cJSON_NULL)
		return Value();
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Unsupported JSON type."));
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
	switch (GetType()) {
		case ValueNumber:
			return cJSON_CreateNumber(boost::get<double>(m_Value));

		case ValueString:
			return cJSON_CreateString(boost::get<String>(m_Value).CStr());

		case ValueObject:
			if (IsObjectType<Dictionary>()) {
				Dictionary::Ptr dictionary = *this;
				return dictionary->ToJson();
			} else if (IsObjectType<Array>()) {
				Array::Ptr array = *this;
				return array->ToJson();
			} else {
				Log(LogDebug, "base", "Ignoring unknown object while converting variant to JSON.");
				return cJSON_CreateNull();
			}

		case ValueEmpty:
			return cJSON_CreateNull();

		default:
			BOOST_THROW_EXCEPTION(std::runtime_error("Invalid variant type."));
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
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid JSON String: " + jsonString));

	Value value = FromJson(json);
	cJSON_Delete(json);

	return value;
}

/**
 * Returns the type of the value.
 *
 * @returns The type.
 */
ValueType Value::GetType(void) const
{
	return static_cast<ValueType>(m_Value.which());
}

Value icinga::operator+(const Value& lhs, const char *rhs)
{
	return static_cast<String>(lhs) + rhs;
}

Value icinga::operator+(const char *lhs, const Value& rhs)
{
	return lhs + static_cast<String>(rhs);
}

std::ostream& icinga::operator<<(std::ostream& stream, const Value& value)
{
	stream << static_cast<String>(value);
	return stream;
}

std::istream& icinga::operator>>(std::istream& stream, Value& value)
{
	String tstr;
	stream >> tstr;
	value = tstr;
	return stream;
}
