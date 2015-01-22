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

using namespace icinga;

Type::TypeMap& Type::GetTypes(void)
{
	static TypeMap types;

	return types;
}

void Type::Register(const Type::Ptr& type)
{
	VERIFY(GetByName(type->GetName()) == NULL);

	GetTypes()[type->GetName()] = type;
}

Type::Ptr Type::GetByName(const String& name)
{
	std::map<String, Type::Ptr>::const_iterator it;

	it = GetTypes().find(name);

	if (it == GetTypes().end())
		return Type::Ptr();

	return it->second;
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

