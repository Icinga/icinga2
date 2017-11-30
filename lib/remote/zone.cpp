/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include "base/logger.hpp"

using namespace icinga;

REGISTER_TYPE(Zone);

void Zone::OnAllConfigLoaded(void)
{
	ObjectImpl<Zone>::OnAllConfigLoaded();

	m_Parent = Zone::GetByName(GetParentRaw());

	Zone::Ptr zone = m_Parent;
	int levels = 0;

	Array::Ptr endpoints = GetEndpointsRaw();

	if (endpoints) {
		ObjectLock olock(endpoints);
		for (const String& endpoint : endpoints) {
			Endpoint::Ptr ep = Endpoint::GetByName(endpoint);

			if (ep)
				ep->SetCachedZone(this);
		}
	}

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

		for (const String& name : endpoints) {
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

	if (object_zone->GetGlobal())
		return true;

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

bool Zone::IsSingleInstance(void) const
{
	Array::Ptr endpoints = GetEndpointsRaw();
	return !endpoints || endpoints->GetLength() < 2;
}

Zone::Ptr Zone::GetLocalZone(void)
{
	Endpoint::Ptr local = Endpoint::GetLocalEndpoint();

	if (!local)
		return nullptr;

	return local->GetZone();
}

void Zone::ValidateEndpointsRaw(const Array::Ptr& value, const ValidationUtils& utils)
{
	ObjectImpl<Zone>::ValidateEndpointsRaw(value, utils);

	if (value && value->GetLength() > 2) {
		Log(LogWarning, "Zone")
		    << "The Zone object '" << GetName() << "' has more than two endpoints."
		    << " Due to a known issue this type of configuration is strongly"
		    << " discouraged and may cause Icinga to use excessive amounts of CPU time.";
	}
}
