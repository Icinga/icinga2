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

Host::Host(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	RegisterAttribute("alias", Attribute_Config);
	RegisterAttribute("hostgroups", Attribute_Config);
}

String Host::GetAlias(void) const
{
	String value;
	if (GetAttribute("alias", &value))
		return value;
	else
		return GetName();
}

bool Host::Exists(const String& name)
{
	return (DynamicObject::GetObject("Host", name));
}

Host::Ptr Host::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("Host", name);

	if (!configObject)
		throw_exception(invalid_argument("Host '" + name + "' does not exist."));

	return dynamic_pointer_cast<Host>(configObject);
}

Dictionary::Ptr Host::GetGroups(void) const
{
	Dictionary::Ptr value;
	GetAttribute("hostgroups", &value);
	return value;
}

set<String> Host::GetParents(void)
{
	set<String> parents;

	Dictionary::Ptr dependencies;
	GetAttribute("dependencies", &dependencies);
	if (dependencies) {
		dependencies = Service::ResolveDependencies(GetSelf(), dependencies);

		Value dependency;
		BOOST_FOREACH(tie(tuples::ignore, dependency), dependencies) {
			Service::Ptr service = Service::GetByName(dependency);

			String parent = service->GetHost()->GetName();

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
	GetAttribute("macros", &value);
	return value;
}

bool Host::IsReachable(void)
{
	Dictionary::Ptr dependencies;
	GetAttribute("dependencies", &dependencies);
	if (dependencies) {
		dependencies = Service::ResolveDependencies(GetSelf(), dependencies);

		Value dependency;
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
	GetAttribute("hostchecks", &hostchecks);
	if (hostchecks) {
		hostchecks = Service::ResolveDependencies(GetSelf(), hostchecks);

		Value hostcheck;
		BOOST_FOREACH(tie(tuples::ignore, hostcheck), hostchecks) {
			Service::Ptr service = Service::GetByName(hostcheck);

			if (service->GetState() != StateOK && service->GetState() != StateWarning) {
				return false;
			}
		}
	}

	return true;
}
