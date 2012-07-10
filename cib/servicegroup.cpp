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

#include "i2-cib.h"

using namespace icinga;

ServiceGroup::ServiceGroup(const ConfigObject::Ptr& configObject)
	: ConfigObjectAdapter(configObject)
{
	assert(GetType() == "servicegroup");
}


string ServiceGroup::GetAlias(void) const
{
	string value;

	if (GetProperty("alias", &value))
		return value;

	return GetName();
}

string ServiceGroup::GetNotesUrl(void) const
{
	string value;
	GetProperty("notes_url", &value);
	return value;
}

string ServiceGroup::GetActionUrl(void) const
{
	string value;
	GetProperty("action_url", &value);
	return value;
}

bool ServiceGroup::Exists(const string& name)
{
	return (ConfigObject::GetObject("hostgroup", name));
}

ServiceGroup ServiceGroup::GetByName(const string& name)
{
	ConfigObject::Ptr configObject = ConfigObject::GetObject("hostgroup", name);

	if (!configObject)
		throw invalid_argument("ServiceGroup '" + name + "' does not exist.");

	return ServiceGroup(configObject);
}

