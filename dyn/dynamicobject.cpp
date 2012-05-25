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

#include "i2-dyn.h"

using namespace icinga;

DynamicDictionary::Ptr DynamicObject::GetProperties(void) const
{
	return m_Properties;
}

void DynamicObject::SetProperties(DynamicDictionary::Ptr properties)
{	
	m_Properties = properties;
	Dictionary::Ptr resolvedProperties = properties->ToFlatDictionary();
	Reload(resolvedProperties);
}

string DynamicObject::GetName(void) const
{
	return m_Name;
}

void DynamicObject::SetName(string name)
{
	m_Name = name;
}

string DynamicObject::GetType(void) const
{
	return m_Type;
}

void DynamicObject::SetType(string type)
{
	m_Type = type;
}

bool DynamicObject::IsLocal(void) const
{
	return m_Local;
}

void DynamicObject::SetLocal(bool value)
{
	m_Local = value;
}

bool DynamicObject::IsAbstract(void) const
{
	return m_Abstract;
}

void DynamicObject::SetAbstract(bool value)
{
	m_Abstract = value;
}

void DynamicObject::Commit(void)
{
	// TODO: link properties to parent objects

	Dictionary::Ptr resolvedProperties = m_Properties->ToFlatDictionary();
	Reload(resolvedProperties);
}

void DynamicObject::Reload(Dictionary::Ptr resolvedProperties)
{
	resolvedProperties->GetProperty("__name", &m_Name);
	resolvedProperties->GetProperty("__type", &m_Type);
	resolvedProperties->GetProperty("__local", &m_Local);
	resolvedProperties->GetProperty("__abstract", &m_Abstract);
}
