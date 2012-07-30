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

REGISTER_CLASS(Host);

string Host::GetAlias(void) const
{
	string value;

	if (GetProperty("alias", &value))
		return value;

	return GetName();
}

bool Host::Exists(const string& name)
{
	return (DynamicObject::GetObject("Host", name));
}

Host::Ptr Host::GetByName(const string& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("Host", name);

	if (!configObject)
		throw_exception(invalid_argument("Host '" + name + "' does not exist."));

	return dynamic_pointer_cast<Host>(configObject);
}

Dictionary::Ptr Host::GetGroups(void) const
{
	Dictionary::Ptr value;
	GetProperty("hostgroups", &value);
	return value;
}

set<string> Host::GetParents(void)
{
	set<string> parents;

	Dictionary::Ptr dependencies;

	if (GetProperty("dependencies", &dependencies)) {
		dependencies = Service::ResolveDependencies(GetSelf(), dependencies);

		Variant dependency;
		BOOST_FOREACH(tie(tuples::ignore, dependency), dependencies) {
			Service::Ptr service = Service::GetByName(dependency);

			string parent = service->GetHost()->GetName();

			/* ignore ourselves */
			if (parent == GetName())
				continue;

			parents.insert(parent);
		}
	}

	return parents;
}

Dictionary::Ptr Host::GetMacros(void) const
{
	Dictionary::Ptr value;
	GetProperty("macros", &value);
	return value;
}

bool Host::IsReachable(void)
{
	Dictionary::Ptr dependencies;
	if (GetProperty("dependencies", &dependencies)) {
		dependencies = Service::ResolveDependencies(GetSelf(), dependencies);

		Variant dependency;
		BOOST_FOREACH(tie(tuples::ignore, dependency), dependencies) {
			Service::Ptr service = Service::GetByName(dependency);

			if (!service->IsReachable() ||
			    (service->GetState() != StateOK && service->GetState() != StateWarning)) {
				return false;
			}
		}
	}

	return true;
}

bool Host::IsUp(void)
{
	Dictionary::Ptr hostchecks;
	if (GetProperty("hostchecks", &hostchecks)) {
		hostchecks = Service::ResolveDependencies(GetSelf(), hostchecks);

		Variant hostcheck;
		BOOST_FOREACH(tie(tuples::ignore, hostcheck), hostchecks) {
			Service::Ptr service = Service::GetByName(hostcheck);

			if (service->GetState() != StateOK && service->GetState() != StateWarning) {
				return false;
			}
		}
	}

	return true;
}
