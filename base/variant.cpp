/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-base.h"

using namespace icinga;

void Variant::Convert(VariantType newType) const
{
	if (newType == m_Type)
		return;

	if (m_Type == VariantString && newType == VariantInteger) {
		m_IntegerValue = strtol(m_StringValue.c_str(), NULL, 10);
		m_Type = VariantInteger;

		return;
	}

	// TODO: convert variant data
	throw InvalidArgumentException("Invalid variant conversion.");
}

VariantType Variant::GetType(void) const
{
	return m_Type;
}

long Variant::GetInteger(void) const
{
	Convert(VariantInteger);

	return m_IntegerValue;
}

string Variant::GetString(void) const
{
	Convert(VariantString);

	return m_StringValue;
}

Object::Ptr Variant::GetObject(void) const
{
	Convert(VariantObject);

	return m_ObjectValue;
}

bool Variant::IsEmpty(void) const
{
	return (m_Type == VariantEmpty);
}

Variant::operator long(void) const
{
	return GetInteger();
}

Variant::operator string(void) const
{
	return GetString();
}

Variant::operator Object::Ptr(void) const
{
	return GetObject();
}
