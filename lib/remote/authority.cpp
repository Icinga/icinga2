/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/zone.hpp"
#include "remote/apilistener.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"

using namespace icinga;

void ApiListener::UpdateObjectAuthority()
{
	Zone::Ptr my_zone = Zone::GetLocalZone();

	std::vector<Endpoint::Ptr> endpoints;
	Endpoint::Ptr my_endpoint;

	if (my_zone) {
		my_endpoint = Endpoint::GetLocalEndpoint();

		int num_total = 0;

		for (const Endpoint::Ptr& endpoint : my_zone->GetEndpoints()) {
			num_total++;

			if (endpoint != my_endpoint && !endpoint->GetConnected())
				continue;

			endpoints.push_back(endpoint);
		}

		double mainTime = Application::GetMainTime();

		if (num_total > 1 && endpoints.size() <= 1 && (mainTime == 0 || Utility::GetTime() - mainTime < 60))
			return;

		std::sort(endpoints.begin(), endpoints.end(),
			[](const ConfigObject::Ptr& a, const ConfigObject::Ptr& b) {
				return a->GetName() < b->GetName();
			}
		);
	}

	for (const Type::Ptr& type : Type::GetAllTypes()) {
		auto *dtype = dynamic_cast<ConfigType *>(type.get());

		if (!dtype)
			continue;

		for (const ConfigObject::Ptr& object : dtype->GetObjects()) {
			if (!object->IsActive() || object->GetHAMode() != HARunOnce)
				continue;

			bool authority;

			if (!my_zone)
				authority = true;
			else
				authority = endpoints[Utility::SDBM(object->GetName()) % endpoints.size()] == my_endpoint;

			object->SetAuthority(authority);
		}
	}
}
