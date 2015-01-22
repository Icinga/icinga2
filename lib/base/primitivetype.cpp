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

#include "base/primitivetype.hpp"
#include "base/dictionary.hpp"

using namespace icinga;

PrimitiveType::PrimitiveType(const String& name)
	: m_Name(name)
{ }

String PrimitiveType::GetName(void) const
{
	return m_Name;
}

Type::Ptr PrimitiveType::GetBaseType(void) const
{
	return Type::Ptr();
}

int PrimitiveType::GetAttributes(void) const
{
	return 0;
}

int PrimitiveType::GetFieldId(const String& name) const
{
	return -1;
}

Field PrimitiveType::GetFieldInfo(int id) const
{
	throw std::runtime_error("Invalid field ID.");
}

int PrimitiveType::GetFieldCount(void) const
{
	return 0;
}

ObjectFactory PrimitiveType::GetFactory(void) const
{
	return NULL;
}

