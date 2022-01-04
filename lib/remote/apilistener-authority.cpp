/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/zone.hpp"
#include "remote/apilistener.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"

using namespace icinga;

std::atomic<bool> ApiListener::m_UpdatedObjectAuthority (false);

void ApiListener::UpdateObjectAuthority()
{
	/* Always run this, even if there is no 'api' feature enabled. */
	if (auto listener = ApiListener::GetInstance()) {
		Log(LogNotice, "ApiListener")
			<< "Updating object authority for objects at endpoint '" << listener->GetIdentity() << "'.";
	} else {
		Log(LogNotice, "ApiListener")
			<< "Updating object authority for local objects.";
	}

	Zone::Ptr my_zone = Zone::GetLocalZone();

	std::vector<Endpoint::Ptr> endpoints;
	Endpoint::Ptr my_endpoint;

	if (my_zone) {
		my_endpoint = Endpoint::GetLocalEndpoint();

		int num_total = 0;

		for (const auto& endpoint : my_zone->GetEndpoints()) {
			num_total++;

			if (endpoint != my_endpoint && !endpoint->GetConnected())
				continue;

			endpoints.push_back(endpoint);
		}

		double startTime = Application::GetStartTime();

		/* 30 seconds cold startup, don't update any authority to give the secondary endpoint time to reconnect. */
		if (num_total > 1 && endpoints.size() <= 1 && (startTime == 0 || Utility::GetTime() - startTime < 30))
			return;

		std::sort(endpoints.begin(), endpoints.end(),
			[](const ConfigObject::Ptr& a, const ConfigObject::Ptr& b) {
				return a->GetName() < b->GetName();
			}
		);
	}

	for (const auto& type : Type::GetAllTypes()) {
		auto *dtype = dynamic_cast<ConfigType *>(type.get());

		if (!dtype)
			continue;

		for (const auto& object : dtype->GetObjects()) {
			if (!object->IsActive() || object->GetHAMode() != HARunOnce)
				continue;

			bool authority;

			if (!my_zone)
				authority = true;
			else
				authority = endpoints[Utility::SDBM(object->GetName()) % endpoints.size()] == my_endpoint;

#ifdef I2_DEBUG
// 			//Enable on demand, causes heavy logging on each run.
//			Log(LogDebug, "ApiListener")
//				<< "Setting authority '" << Convert::ToString(authority) << "' for object '" << object->GetName() << "' of type '" << object->GetReflectionType()->GetName() << "'.";
#endif /* I2_DEBUG */

			object->SetAuthority(authority);
		}
	}

	m_UpdatedObjectAuthority.store(true);
}
