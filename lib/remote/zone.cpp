/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "remote/zone.hpp"
#include "remote/zone.tcpp"
#include "remote/jsonrpcconnection.hpp"
#include "base/objectlock.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Zone);

void Zone::OnAllConfigLoaded(void)
{
	m_Parent = Zone::GetByName(GetParentRaw());

	Zone::Ptr zone = m_Parent;
	int levels = 0;

	while (zone) {
		m_AllParents.push_back(zone);

		zone = Zone::GetByName(zone->GetParentRaw());
		levels++;

		if (levels > 32)
			BOOST_THROW_EXCEPTION(ScriptError("Infinite recursion detected while resolving zone graph. Check your zone hierarchy.", GetDebugInfo()));
	}
}

Zone::Ptr Zone::GetParent(void) const
{
	return m_Parent;
}

std::set<Endpoint::Ptr> Zone::GetEndpoints(void) const
{
	std::set<Endpoint::Ptr> result;

	Array::Ptr endpoints = GetEndpointsRaw();

	if (endpoints) {
		ObjectLock olock(endpoints);

		BOOST_FOREACH(const String& name, endpoints) {
			Endpoint::Ptr endpoint = Endpoint::GetByName(name);

			if (!endpoint)
				continue;

			result.insert(endpoint);
		}
	}

	return result;
}

std::vector<Zone::Ptr> Zone::GetAllParents(void) const
{
	return m_AllParents;
}

bool Zone::CanAccessObject(const ConfigObject::Ptr& object)
{
	Zone::Ptr object_zone;

	if (object->GetReflectionType() == Zone::TypeInstance)
		object_zone = static_pointer_cast<Zone>(object);
	else
		object_zone = static_pointer_cast<Zone>(object->GetZone());

	if (!object_zone)
		object_zone = Zone::GetLocalZone();

	return object_zone->IsChildOf(this);
}

bool Zone::IsChildOf(const Zone::Ptr& zone)
{
	Zone::Ptr azone = this;

	while (azone) {
		if (azone == zone)
			return true;

		azone = azone->GetParent();
	}

	return false;
}

bool Zone::IsGlobal(void) const
{
	return GetGlobal();
}

Zone::Ptr Zone::GetLocalZone(void)
{
	Endpoint::Ptr local = Endpoint::GetLocalEndpoint();

	if (!local)
		return Zone::Ptr();

	return local->GetZone();
}

