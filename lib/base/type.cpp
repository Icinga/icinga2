/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "base/type.hpp"
#include "base/scriptglobal.hpp"

using namespace icinga;

Type::Ptr Type::TypeInstance;

static void RegisterTypeType(void)
{
	Type::Ptr type = new TypeType();
	type->SetPrototype(TypeType::GetPrototype());
	Type::TypeInstance = type;
	Type::Register(type);
}

INITIALIZE_ONCE(RegisterTypeType);

String Type::ToString(void) const
{
	return "type '" + GetName() + "'";
}

void Type::Register(const Type::Ptr& type)
{
	VERIFY(GetByName(type->GetName()) == NULL);

	ScriptGlobal::Set(type->GetName(), type);
}

Type::Ptr Type::GetByName(const String& name)
{
	Value ptype = ScriptGlobal::Get(name, &Empty);

	if (!ptype.IsObjectType<Type>())
		return Type::Ptr();

	return ptype;
}

String Type::GetPluralName(void) const
{
	String name = GetName();

	if (name.GetLength() >= 2 && name[name.GetLength() - 1] == 'y' &&
	    name.SubStr(name.GetLength() - 2, 1).FindFirstOf("aeiou") == String::NPos)
		return name.SubStr(0, name.GetLength() - 1) + "ies";
	else
		return name + "s";
}

Object::Ptr Type::Instantiate(void) const
{
	ObjectFactory factory = GetFactory();

	if (!factory)
		BOOST_THROW_EXCEPTION(std::runtime_error("Type does not have a factory function."));

	return factory();
}

bool Type::IsAbstract(void) const
{
	return ((GetAttributes() & TAAbstract) != 0);
}

bool Type::IsAssignableFrom(const Type::Ptr& other) const
{
	for (Type::Ptr t = other; t; t = t->GetBaseType()) {
		if (t.get() == this)
			return true;
	}

	return false;
}

Object::Ptr Type::GetPrototype(void) const
{
	return m_Prototype;
}

void Type::SetPrototype(const Object::Ptr& object)
{
	m_Prototype = object;
}

void Type::SetField(int id, const Value& value, bool suppress_events, const Value& cookie)
{
	if (id == 1) {
		SetPrototype(value);
		return;
	}

	Object::SetField(id, value, suppress_events, cookie);
}

Value Type::GetField(int id) const
{
	int real_id = id - Object::TypeInstance->GetFieldCount();
	if (real_id < 0)
		return Object::GetField(id);

	if (real_id == 0)
		return GetName();
	else if (real_id == 1)
		return GetPrototype();
	else if (real_id == 2)
		return GetBaseType();

	BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

std::vector<String> Type::GetLoadDependencies(void) const
{
	return std::vector<String>();
}

void Type::RegisterAttributeHandler(int fieldId, const AttributeHandler& callback)
{
	throw std::runtime_error("Invalid field ID.");
}

String TypeType::GetName(void) const
{
	return "Type";
}

Type::Ptr TypeType::GetBaseType(void) const
{
	return Object::TypeInstance;
}

int TypeType::GetAttributes(void) const
{
	return 0;
}

int TypeType::GetFieldId(const String& name) const
{
	int base_field_count = GetBaseType()->GetFieldCount();

	if (name == "name")
		return base_field_count + 0;
	else if (name == "prototype")
		return base_field_count + 1;
	else if (name == "base")
		return base_field_count + 2;

	return GetBaseType()->GetFieldId(name);
}

Field TypeType::GetFieldInfo(int id) const
{
	int real_id = id - GetBaseType()->GetFieldCount();
	if (real_id < 0)
		return GetBaseType()->GetFieldInfo(id);

	if (real_id == 0)
		return Field(0, "String", "name", "", NULL, 0, 0);
	else if (real_id == 1)
		return Field(1, "Object", "prototype", "", NULL, 0, 0);
	else if (real_id == 2)
		return Field(2, "Type", "base", "", NULL, 0, 0);

	throw std::runtime_error("Invalid field ID.");
}

int TypeType::GetFieldCount(void) const
{
	return GetBaseType()->GetFieldCount() + 3;
}

ObjectFactory TypeType::GetFactory(void) const
{
	return NULL;
}

