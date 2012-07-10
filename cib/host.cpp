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

Host::Host(const ConfigObject::Ptr& configObject)
	: ConfigObjectAdapter(configObject)
{
	assert(GetType() == "host");
}


string Host::GetAlias(void) const
{
	string value;

	if (GetProperty("alias", &value))
		return value;

	return GetName();
}

bool Host::Exists(const string& name)
{
	return (ConfigObject::GetObject("host", name));
}

Host Host::GetByName(const string& name)
{
	ConfigObject::Ptr configObject = ConfigObject::GetObject("host", name);

	if (!configObject)
		throw invalid_argument("Host '" + name + "' does not exist.");

	return Host(configObject);
}

Dictionary::Ptr Host::GetGroups(void) const
{
	Dictionary::Ptr value;
	GetProperty("hostgroups", &value);
	return value;
}

set<string> Host::GetParents(void) const
{
	set<string> parents;

	Dictionary::Ptr dependencies;

	if (GetProperty("dependencies", &dependencies)) {
		dependencies = Service::ResolveDependencies(*this, dependencies);

		Dictionary::Iterator it;
		for (it = dependencies->Begin(); it != dependencies->End(); it++) {
			Service service = Service::GetByName(it->second);

			string parent = service.GetHost().GetName();

			/* ignore ourselves */
			if (parent == GetName())
				continue;

			parents.insert(parent);
		}
	}

	return parents;
}

bool Host::IsReachable(void) const
{
	Dictionary::Ptr dependencies;
	if (GetProperty("dependencies", &dependencies)) {
		dependencies = Service::ResolveDependencies(*this, dependencies);

		Dictionary::Iterator it;
		for (it = dependencies->Begin(); it != dependencies->End(); it++) {
			Service service = Service::GetByName(it->second);

			if (!service.IsReachable() ||
			    (service.GetState() != StateOK && service.GetState() != StateWarning)) {
				return false;
			}
		}
	}

	return true;
}

bool Host::IsUp(void) const
{
	Dictionary::Ptr hostchecks;
	if (GetProperty("hostchecks", &hostchecks)) {
		hostchecks = Service::ResolveDependencies(*this, hostchecks);

		Dictionary::Iterator it;
		for (it = hostchecks->Begin(); it != hostchecks->End(); it++) {
			Service service = Service::GetByName(it->second);
			if (service.GetState() != StateOK && service.GetState() != StateWarning) {
				return false;
			}
		}
	}

	return true;
}
