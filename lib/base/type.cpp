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

#include "base/type.h"

using namespace icinga;

Type::TypeMap& Type::GetTypes(void)
{
	static TypeMap types;

	return types;
}

void Type::Register(const Type *type)
{
	VERIFY(GetByName(type->GetName()) == NULL);

	GetTypes()[type->GetName()] = type;
}

const Type *Type::GetByName(const String& name)
{
	std::map<String, const Type *>::const_iterator it;

	it = GetTypes().find(name);

	if (it == GetTypes().end())
		return NULL;

	return it->second;
}

Object::Ptr Type::Instantiate(void) const
{
	return m_Factory();
}

bool Type::IsAbstract(void) const
{
	return GetAttributes() & TAAbstract;
}

bool Type::IsSafe(void) const
{
	return GetAttributes() & TASafe;
}

bool Type::IsAssignableFrom(const Type *other) const
{
	for (const Type *t = other; t; t = t->GetBaseType()) {
		if (t == this)
			return true;
	}

	return false;
}

void Type::SetFactory(const Type::Factory& factory)
{
	m_Factory = factory;
}
