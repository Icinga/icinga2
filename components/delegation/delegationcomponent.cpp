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

#include "delegation/delegationcomponent.h"
#include "remoting/endpointmanager.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include <algorithm>
#include "base/dynamictype.h"
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

using namespace icinga;

REGISTER_TYPE(DelegationComponent);

DelegationComponent::DelegationComponent(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{ }

void DelegationComponent::Start(void)
{
	m_DelegationTimer = boost::make_shared<Timer>();

	m_DelegationTimer->SetInterval(30);
	m_DelegationTimer->OnTimerExpired.connect(boost::bind(&DelegationComponent::DelegationTimerHandler, this));
	m_DelegationTimer->Start();
	m_DelegationTimer->Reschedule(Utility::GetTime() + 10);
}

bool DelegationComponent::IsEndpointChecker(const Endpoint::Ptr& endpoint)
{
	return (endpoint->HasSubscription("checker"));
}

std::set<Endpoint::Ptr> DelegationComponent::GetCheckerCandidates(const Service::Ptr& service) const
{
	std::set<Endpoint::Ptr> candidates;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Endpoint")) {
		Endpoint::Ptr endpoint = dynamic_pointer_cast<Endpoint>(object);
		String myIdentity = EndpointManager::GetInstance()->GetIdentity();

		/* ignore local-only endpoints (unless this is a local-only instance) */
		if (endpoint->IsLocal() && !myIdentity.IsEmpty())
			continue;

		/* ignore disconnected endpoints */
		if (!endpoint->IsConnected() && endpoint->GetName() != myIdentity)
			continue;

		/* ignore endpoints that aren't running the checker component */
		if (!IsEndpointChecker(endpoint))
			continue;

		/* ignore endpoints that aren't allowed to check this service */
		if (!service->IsAllowedChecker(endpoint->GetName()))
			continue;

		candidates.insert(endpoint);
	}

	return candidates;
}

void DelegationComponent::DelegationTimerHandler(void)
{
	std::map<Endpoint::Ptr, int> histogram;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Endpoint")) {
		Endpoint::Ptr endpoint = dynamic_pointer_cast<Endpoint>(object);

		histogram[endpoint] = 0;
	}

	std::vector<Service::Ptr> services;

	/* build "checker -> service count" histogram */
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);

		if (!service)
			continue;

		services.push_back(service);

		String checker = service->GetCurrentChecker();

		if (checker.IsEmpty())
			continue;

		Endpoint::Ptr endpoint = Endpoint::GetByName(checker);

		if (!endpoint)
			continue;

		histogram[endpoint]++;
	}

	int delegated = 0;

	/* re-assign services */
	BOOST_FOREACH(const Service::Ptr& service, services) {
		String checker = service->GetCurrentChecker();

		Endpoint::Ptr oldEndpoint = Endpoint::GetByName(checker);

		std::set<Endpoint::Ptr> candidates = GetCheckerCandidates(service);

		int avg_services = 0, overflow_tolerance = 0;
		std::vector<Endpoint::Ptr>::iterator cit;

		if (!candidates.empty()) {
#ifdef _DEBUG
			std::ostringstream msgbuf;
			msgbuf << "Service: " << service->GetName() << ", candidates: " << candidates.size();
			Log(LogDebug, "delegation", msgbuf.str());
#endif /* _DEBUG */

			BOOST_FOREACH(const Endpoint::Ptr& candidate, candidates) {
				avg_services += histogram[candidate];
			}

			avg_services /= candidates.size();
			overflow_tolerance = candidates.size() * 2;
		}

		/* don't re-assign service if the checker is still valid
		 * and doesn't have too many services */

		if (oldEndpoint && oldEndpoint->IsConnected() &&
		    candidates.find(oldEndpoint) != candidates.end() &&
		    histogram[oldEndpoint] <= avg_services + overflow_tolerance)
			continue;

		/* clear the service's current checker */
		if (!checker.IsEmpty()) {
			{
				ObjectLock olock(service);
				service->SetCurrentChecker("");
			}

			if (oldEndpoint)
				histogram[oldEndpoint]--;
		}

		/* find a new checker for the service */
		BOOST_FOREACH(const Endpoint::Ptr& candidate, candidates) {
			/* does this checker already have too many services */
			if (histogram[candidate] > avg_services)
				continue;

			{
				ObjectLock olock(service);
				service->SetCurrentChecker(candidate->GetName());
			}

			histogram[candidate]++;

			/* reschedule the service; this avoids "check floods"
			 * when a lot of services are re-assigned that haven't
			 * been checked recently. */
			service->UpdateNextCheck();

			delegated++;

			break;
		}

		if (candidates.empty()) {
			if (service->GetState() != StateUncheckable && service->GetEnableActiveChecks()) {
				Dictionary::Ptr cr = boost::make_shared<Dictionary>();

				double now = Utility::GetTime();
				cr->Set("schedule_start", now);
				cr->Set("schedule_end", now);
				cr->Set("execution_start", now);
				cr->Set("execution_end", now);

				cr->Set("state", StateUncheckable);
				cr->Set("output", "No checker is available for this service.");

				cr->Seal();

				service->ProcessCheckResult(cr);

				Log(LogWarning, "delegation", "Can't delegate service: " + service->GetName());
			}

			continue;
		}

		ASSERT(!service->GetCurrentChecker().IsEmpty());
	}

	Endpoint::Ptr endpoint;
	int count;
	BOOST_FOREACH(boost::tie(endpoint, count), histogram) {
		std::ostringstream msgbuf;
		msgbuf << "histogram: " << endpoint->GetName() << " - " << count;
		Log(LogInformation, "delegation", msgbuf.str());
	}

	std::ostringstream msgbuf;
	msgbuf << "Updated delegations for " << delegated << " services";
	Log(LogInformation, "delegation", msgbuf.str());
}
