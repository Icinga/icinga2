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

/**
 * ConfigObject
 *
 * Constructor for the ConfigObject class.
 *
 * @param type The type of the object.
 * @param name The name of the object.
 */
ConfigObject::ConfigObject(const string& type, const string& name)
{
	m_Type = type;
	m_Name = name;
	m_Replicated = false;
}

/**
 * SetHive
 *
 * Sets the hive this object belongs to.
 *
 * @param hive The hive.
 */
void ConfigObject::SetHive(const ConfigHive::WeakPtr& hive)
{
	if (m_Hive.lock())
		throw InvalidArgumentException("Config object already has a parent hive.");

	m_Hive = hive;
}

/**
 * GetHive
 *
 * Retrieves the hive this object belongs to.
 *
 * @returns The hive.
 */
ConfigHive::WeakPtr ConfigObject::GetHive(void) const
{
	return m_Hive;
}

/**
 * SetName
 *
 * Sets the name of this object.
 *
 * @param name The name.
 */
void ConfigObject::SetName(const string& name)
{
	m_Name = name;
}

/**
 * GetName
 *
 * Retrieves the name of this object.
 *
 * @returns The name.
 */
string ConfigObject::GetName(void) const
{
	return m_Name;
}

/**
 * SetType
 *
 * Sets the type of this object.
 *
 * @param type The type.
 */
void ConfigObject::SetType(const string& type)
{
	m_Type = type;
}

/**
 * GetType
 *
 * Retrieves the type of this object.
 *
 * @returns The type.
 */
string ConfigObject::GetType(void) const
{
	return m_Type;
}

/**
 * SetReplicated
 *
 * Sets whether this object was replicated.
 *
 * @param replicated Whether this object was replicated.
 */
void ConfigObject::SetReplicated(bool replicated)
{
	m_Replicated = replicated;
}

/**
 * GetReplicated
 *
 * Retrieves whether this object was replicated.
 *
 * @returns Whether this object was replicated.
 */
bool ConfigObject::GetReplicated(void) const
{
	return m_Replicated;
}

/**
 * Commit
 *
 * Handles changed properties by propagating them to the hive
 * and collection this object is contained in.
 *
 */
void ConfigObject::Commit(void)
{
	ConfigHive::Ptr hive = m_Hive.lock();
	if (hive) {
		EventArgs ea;
		ea.Source = shared_from_this();
		hive->GetCollection(m_Type)->OnObjectCommitted(ea);
		hive->OnObjectCommitted(ea);
	}
}
