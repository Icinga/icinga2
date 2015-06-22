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

#include "remote/zone.hpp"
#include "remote/zone.tcpp"
#include "remote/jsonrpcconnection.hpp"
#include "base/objectlock.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Zone);

Zone::Ptr Zone::GetParent(void) const
{
	return Zone::GetByName(GetParentRaw());
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

bool Zone::CanAccessObject(const DynamicObject::Ptr& object)
{
	Zone::Ptr object_zone;

	if (dynamic_pointer_cast<Zone>(object))
		object_zone = static_pointer_cast<Zone>(object);
	else
		object_zone = Zone::GetByName(object->GetZoneName());

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
