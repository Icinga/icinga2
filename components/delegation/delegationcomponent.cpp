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

#include "i2-delegation.h"
#include <algorithm>

using namespace icinga;

EXPORT_COMPONENT(delegation, DelegationComponent);

void DelegationComponent::Start(void)
{
	m_DelegationTimer = boost::make_shared<Timer>();
	// TODO: implement a handler for config changes for the delegation_interval variable
	m_DelegationTimer->SetInterval(GetDelegationInterval());
	m_DelegationTimer->Reschedule(Utility::GetTime() + 10);
	m_DelegationTimer->OnTimerExpired.connect(boost::bind(&DelegationComponent::DelegationTimerHandler, this));
	m_DelegationTimer->Start();
}

double DelegationComponent::GetDelegationInterval(void) const
{
	Value interval = GetConfig()->Get("delegation_interval");
	if (interval.IsEmpty())
		return 30;
	else
		return interval;
}

bool DelegationComponent::IsEndpointChecker(const Endpoint::Ptr& endpoint)
{
	return (endpoint->HasSubscription("checker"));
}

vector<Endpoint::Ptr> DelegationComponent::GetCheckerCandidates(const Service::Ptr& service) const
{
	vector<Endpoint::Ptr> candidates;

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Endpoint")->GetObjects()) {
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

		candidates.push_back(endpoint);
	}

	return candidates;
}

void DelegationComponent::DelegationTimerHandler(void)
{
	map<Endpoint::Ptr, int> histogram;

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Endpoint")->GetObjects()) {
		Endpoint::Ptr endpoint = dynamic_pointer_cast<Endpoint>(object);

		histogram[endpoint] = 0;
	}

	vector<Service::Ptr> services;

	/* build "checker -> service count" histogram */
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Service")->GetObjects()) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);

		if (!service)
			continue;

		services.push_back(service);

		String checker = service->GetChecker();
		if (checker.IsEmpty())
			continue;

		if (!Endpoint::Exists(checker))
			continue;

		Endpoint::Ptr endpoint = Endpoint::GetByName(checker);

		histogram[endpoint]++;
	}

	std::random_shuffle(services.begin(), services.end());

	int delegated = 0;

	/* re-assign services */
	BOOST_FOREACH(const Service::Ptr& service, services) {
		String checker = service->GetChecker();

		Endpoint::Ptr oldEndpoint;
		if (Endpoint::Exists(checker))
			oldEndpoint = Endpoint::GetByName(checker);

		vector<Endpoint::Ptr> candidates = GetCheckerCandidates(service);

		int avg_services = 0, overflow_tolerance = 0;
		vector<Endpoint::Ptr>::iterator cit;

		if (candidates.size() > 0) {
			std::random_shuffle(candidates.begin(), candidates.end());

			stringstream msgbuf;
			msgbuf << "Service: " << service->GetName() << ", candidates: " << candidates.size();
			Logger::Write(LogDebug, "delegation", msgbuf.str());

			BOOST_FOREACH(const Endpoint::Ptr& candidate, candidates) {
				avg_services += histogram[candidate];
			}

			avg_services /= candidates.size();
			overflow_tolerance = candidates.size() * 2;
		}

		/* don't re-assign service if the checker is still valid
		 * and doesn't have too many services */
		if (oldEndpoint && oldEndpoint->IsConnected() &&
		    find(candidates.begin(), candidates.end(), oldEndpoint) != candidates.end() &&
		    histogram[oldEndpoint] <= avg_services + overflow_tolerance)
			continue;

		/* clear the service's current checker */
		if (!checker.IsEmpty()) {
			service->SetChecker("");

			if (oldEndpoint)
				histogram[oldEndpoint]--;
		}

		/* find a new checker for the service */
		BOOST_FOREACH(const Endpoint::Ptr& candidate, candidates) {
			/* does this checker already have too many services */
			if (histogram[candidate] > avg_services)
				continue;

			service->SetChecker(candidate->GetName());
			histogram[candidate]++;

			/* reschedule the service; this avoids "check floods"
			 * when a lot of services are re-assigned that haven't
			 * been checked recently. */
			service->UpdateNextCheck();

			delegated++;

			break;
		}

		if (candidates.size() == 0) {
			if (service->GetState() != StateUncheckable && service->GetEnableActiveChecks()) {
				Dictionary::Ptr cr = boost::make_shared<Dictionary>();

				double now = Utility::GetTime();
				cr->Set("schedule_start", now);
				cr->Set("schedule_end", now);
				cr->Set("execution_start", now);
				cr->Set("execution_end", now);

				cr->Set("state", StateUncheckable);
				cr->Set("output", "No checker is available for this service.");

				service->ProcessCheckResult(cr);

				Logger::Write(LogWarning, "delegation", "Can't delegate service: " + service->GetName());
			}

			continue;
		}

		assert(!service->GetChecker().IsEmpty());
	}

	Endpoint::Ptr endpoint;
	int count;
	BOOST_FOREACH(tie(endpoint, count), histogram) {
		stringstream msgbuf;
		msgbuf << "histogram: " << endpoint->GetName() << " - " << count;
		Logger::Write(LogInformation, "delegation", msgbuf.str());
	}

	stringstream msgbuf;
	msgbuf << "Updated delegations for " << delegated << " services";
	Logger::Write(LogInformation, "delegation", msgbuf.str());
}
