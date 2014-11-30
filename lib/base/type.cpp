/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

static void RegisterTypeType(void)
{
	Type::Ptr type = new TypeType();
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

Object::Ptr Type::Instantiate(void) const
{
	ObjectFactory factory = GetFactory();

	if (!factory)
		return Object::Ptr();

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

void Type::SetField(int id, const Value& value)
{
	if (id == 0) {
		SetPrototype(value);
		return;
	}

	Object::SetField(id, value);
}

Value Type::GetField(int id) const
{
	if (id == 0)
		return GetPrototype();

	return Object::GetField(id);
}

std::vector<String> Type::GetLoadDependencies(void) const
{
	return std::vector<String>();
}

String TypeType::GetName(void) const
{
	return "Type";
}

Type::Ptr TypeType::GetBaseType(void) const
{
	return Type::Ptr();
}

int TypeType::GetAttributes(void) const
{
	return 0;
}

int TypeType::GetFieldId(const String& name) const
{
	if (name == "prototype")
		return 0;

	return -1;
}

Field TypeType::GetFieldInfo(int id) const
{
	if (id == 0)
		return Field(0, "Object", "prototype", NULL, 0);

	throw std::runtime_error("Invalid field ID.");
}

int TypeType::GetFieldCount(void) const
{
	return 1;
}

ObjectFactory TypeType::GetFactory(void) const
{
	return NULL;
}

