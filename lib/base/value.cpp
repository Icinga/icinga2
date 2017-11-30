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

#include "base/value.hpp"
#include "base/array.hpp"
#include "base/dictionary.hpp"
#include "base/type.hpp"

using namespace icinga;

Value icinga::Empty;

bool Value::ToBool(void) const
{
	switch (GetType()) {
		case ValueNumber:
			return static_cast<bool>(boost::get<double>(m_Value));

		case ValueBoolean:
			return boost::get<bool>(m_Value);

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

String Value::GetTypeName(void) const
{
	Type::Ptr t;

	switch (GetType()) {
		case ValueEmpty:
			return "Empty";
		case ValueNumber:
			return "Number";
		case ValueBoolean:
			return "Boolean";
		case ValueString:
			return "String";
		case ValueObject:
			t = boost::get<Object::Ptr>(m_Value)->GetReflectionType();
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

Type::Ptr Value::GetReflectionType(void) const
{
	switch (GetType()) {
		case ValueEmpty:
			return Object::TypeInstance;
		case ValueNumber:
			return Type::GetByName("Number");
		case ValueBoolean:
			return Type::GetByName("Boolean");
		case ValueString:
			return Type::GetByName("String");
		case ValueObject:
			return boost::get<Object::Ptr>(m_Value)->GetReflectionType();
		default:
			return nullptr;
	}
}

Value Value::Clone(void) const
{
	if (IsObject())
		return static_cast<Object::Ptr>(*this)->Clone();
	else
		return *this;
}

