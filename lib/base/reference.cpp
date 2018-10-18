/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "base/reference.hpp"
#include "base/debug.hpp"
#include "base/primitivetype.hpp"
#include "base/dictionary.hpp"
#include "base/configwriter.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_PRIMITIVE_TYPE_NOINST(Reference, Object, Reference::GetPrototype());

Reference::Reference(const Object::Ptr& parent, const String& index)
	: m_Parent(parent), m_Index(index)
{
}

Value Reference::Get() const
{
	return m_Parent->GetFieldByName(m_Index, true, DebugInfo());
}

void Reference::Set(const Value& value)
{
	m_Parent->SetFieldByName(m_Index, value, false, DebugInfo());
}

Object::Ptr Reference::GetParent() const
{
	return m_Parent;
}

String Reference::GetIndex() const
{
	return m_Index;
}
