/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/zone.hpp"
#include "remote/zone-ti.cpp"
#include "remote/jsonrpcconnection.hpp"
#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_TYPE(Zone);

void Zone::OnAllConfigLoaded()
{
	ObjectImpl<Zone>::OnAllConfigLoaded();

	m_Parent = Zone::GetByName(GetParentRaw());

	if (m_Parent && m_Parent->IsGlobal())
		BOOST_THROW_EXCEPTION(ScriptError("Zone '" + GetName() + "' can not have a global zone as parent.", GetDebugInfo()));

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

Zone::Ptr Zone::GetParent() const
{
	return m_Parent;
}

std::set<Endpoint::Ptr> Zone::GetEndpoints() const
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

std::vector<Zone::Ptr> Zone::GetAllParentsRaw() const
{
	return m_AllParents;
}

Array::Ptr Zone::GetAllParents() const
{
	auto result (new Array);

	for (auto& parent : m_AllParents)
		result->Add(parent->GetName());

	return result;
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

bool Zone::IsGlobal() const
{
	return GetGlobal();
}

bool Zone::IsHACluster() const
{
	auto endpoints = GetEndpointsRaw();
	return endpoints && endpoints->GetLength() >= 2;
}

Zone::Ptr Zone::GetLocalZone()
{
	Endpoint::Ptr local = Endpoint::GetLocalEndpoint();

	if (!local)
		return nullptr;

	return local->GetZone();
}

void Zone::ValidateEndpointsRaw(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Zone>::ValidateEndpointsRaw(lvalue, utils);

	if (lvalue() && lvalue()->GetLength() > 2) {
		Log(LogWarning, "Zone")
			<< "The Zone object '" << GetName() << "' has more than two endpoints."
			<< " Due to a known issue this type of configuration is strongly"
			<< " discouraged and may cause Icinga to use excessive amounts of CPU time.";
	}
}
