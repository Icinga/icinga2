/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/value.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include "base/type.hpp"
#include <cJSON.h>

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
 * Checks whether the variant is a number.
 *
 * @returns true if the variant is a number.
 */
bool Value::IsNumber(void) const
{
	return (GetType() == ValueNumber);
}

/**
 * Checks whether the variant is a string.
 *
 * @returns true if the variant is a string.
 */
bool Value::IsString(void) const
{
	return (GetType() == ValueString);
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

bool Value::ToBool(void) const
{
	switch (GetType()) {
		case ValueNumber:
			return static_cast<bool>(boost::get<double>(m_Value));

		case ValueString:
			return !boost::get<String>(m_Value).IsEmpty();

		case ValueObject:
			if (IsObjectType<Dictionary>()) {
				Dictionary::Ptr dictionary = *this;
				return dictionary->GetLength() > 0;
			} else if (IsObjectType<Array>()) {
				Array::Ptr array = *this;
				return array->GetLength() > 0;
			} else {
				return true;
			}

		case ValueEmpty:
			return false;

		default:
			BOOST_THROW_EXCEPTION(std::runtime_error("Invalid variant type."));
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
				return cJSON_CreateNull();
			}

		case ValueEmpty:
			return cJSON_CreateNull();

		default:
			BOOST_THROW_EXCEPTION(std::runtime_error("Invalid variant type."));
	}
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

String Value::GetTypeName(void) const
{
	const Type *t;

	switch (GetType()) {
		case ValueEmpty:
			return "Empty";
		case ValueNumber:
			return "Number";
		case ValueString:
			return "String";
		case ValueObject:
			t = static_cast<Object::Ptr>(*this)->GetReflectionType();
			if (!t) {
				if (IsObjectType<Array>())
					return "Array";
				else if (IsObjectType<Dictionary>())
					return "Dictionary";
				else
					return "Object";
			} else
				return t->GetName();
		default:
			return "Invalid";
	}
}
