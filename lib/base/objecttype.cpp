/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/objecttype.hpp"
#include "base/initialize.hpp"

using namespace icinga;

INITIALIZE_ONCE_WITH_PRIORITY([]() {
	Type::Ptr type = new ObjectType();
	type->SetPrototype(Object::GetPrototype());
	Type::Register(type);
	Object::TypeInstance = type;
}, 20);

String ObjectType::GetName() const
{
	return "Object";
}

Type::Ptr ObjectType::GetBaseType() const
{
	return nullptr;
}

int ObjectType::GetAttributes() const
{
	return 0;
}

int ObjectType::GetFieldId(const String& name) const
{
	if (name == "type")
		return 0;
	else
		return -1;
}

Field ObjectType::GetFieldInfo(int id) const
{
	if (id == 0)
		return {1, "String", "type", nullptr, nullptr, 0, 0};
	else
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

int ObjectType::GetFieldCount() const
{
	return 1;
}

ObjectFactory ObjectType::GetFactory() const
{
	return DefaultObjectFactory<Object>;
}

