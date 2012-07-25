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

string DelegationComponent::GetName(void) const
{
	return "delegation";
}

void DelegationComponent::Start(void)
{
	m_AllServices = boost::make_shared<ConfigObject::Set>(ConfigObject::GetAllObjects(), ConfigObject::MakeTypePredicate("service"));
	m_AllServices->OnObjectCommitted.connect(boost::bind(&DelegationComponent::ServiceCommittedHandler, this, _2));
	m_AllServices->OnObjectRemoved.connect(boost::bind(&DelegationComponent::ServiceRemovedHandler, this, _2));
	m_AllServices->Start();

	m_DelegationTimer = boost::make_shared<Timer>();
	m_DelegationTimer->SetInterval(30);
	m_DelegationTimer->OnTimerExpired.connect(boost::bind(&DelegationComponent::DelegationTimerHandler, this));
	m_DelegationTimer->Start();
	m_DelegationTimer->Reschedule(0);

	m_Endpoint = boost::make_shared<VirtualEndpoint>();
	m_Endpoint->RegisterPublication("checker::AssignService");
	m_Endpoint->RegisterPublication("checker::ClearServices");
	m_Endpoint->RegisterPublication("delegation::ServiceStatus");
	EndpointManager::GetInstance()->RegisterEndpoint(m_Endpoint);

	EndpointManager::GetInstance()->OnNewEndpoint.connect(bind(&DelegationComponent::NewEndpointHandler, this, _2));
}

void DelegationComponent::Stop(void)
{
	EndpointManager::Ptr mgr = EndpointManager::GetInstance();

	if (mgr)
		mgr->UnregisterEndpoint(m_Endpoint);
}

void DelegationComponent::ServiceCommittedHandler(Service service)
{
	string checker = service.GetChecker();

	if (!checker.empty()) {
		/* object was updated, clear its checker to make sure it's re-delegated by the delegation timer */
		service.SetChecker("");

		/* TODO: figure out a better way to clear individual services */
		Endpoint::Ptr endpoint = EndpointManager::GetInstance()->GetEndpointByIdentity(checker);

		if (endpoint)
			ClearServices(endpoint);
	}
}

void DelegationComponent::ServiceRemovedHandler(Service service)
{
	string checker = service.GetChecker();

	if (!checker.empty()) {
		/* TODO: figure out a better way to clear individual services */
		Endpoint::Ptr endpoint = EndpointManager::GetInstance()->GetEndpointByIdentity(checker);

		if (endpoint)
			ClearServices(endpoint);
	}
}

void DelegationComponent::AssignService(const Endpoint::Ptr& checker, const Service& service)
{
	RequestMessage request;
	request.SetMethod("checker::AssignService");

	MessagePart params;
	params.Set("service", service.GetName());
	request.SetParams(params);

	Logger::Write(LogDebug, "delegation", "Trying to delegate service '" + service.GetName() + "'");

	EndpointManager::GetInstance()->SendUnicastMessage(m_Endpoint, checker, request);
}

void DelegationComponent::ClearServices(const Endpoint::Ptr& checker)
{
	stringstream msgbuf;
	msgbuf << "Clearing assigned services for endpoint '" << checker->GetIdentity() << "'";
	Logger::Write(LogInformation, "delegation", msgbuf.str());

	RequestMessage request;
	request.SetMethod("checker::ClearServices");

	MessagePart params;
	request.SetParams(params);

	EndpointManager::GetInstance()->SendUnicastMessage(m_Endpoint, checker, request);
}

bool DelegationComponent::IsEndpointChecker(const Endpoint::Ptr& endpoint)
{
	return (endpoint->HasSubscription("checker::AssignService"));
}

vector<Endpoint::Ptr> DelegationComponent::GetCheckerCandidates(const Service& service) const
{
	vector<Endpoint::Ptr> candidates;

	EndpointManager::Iterator it;
	for (it = EndpointManager::GetInstance()->Begin(); it != EndpointManager::GetInstance()->End(); it++) {
		Endpoint::Ptr endpoint = it->second;

		/* ignore disconnected endpoints */
		if (!endpoint->IsConnected())
			continue;

		/* ignore endpoints that aren't running the checker component */
		if (!IsEndpointChecker(endpoint))
			continue;

		/* ignore endpoints that aren't allowed to check this service */
		if (!service.IsAllowedChecker(it->first))
			continue;

		candidates.push_back(endpoint);
	}

	return candidates;
}

void DelegationComponent::NewEndpointHandler(const Endpoint::Ptr& endpoint)
{
	endpoint->OnSessionEstablished.connect(bind(&DelegationComponent::SessionEstablishedHandler, this, _1));
}
void DelegationComponent::SessionEstablishedHandler(const Endpoint::Ptr& endpoint)
{
	/* ignore this endpoint if it's not a checker */
	if (!IsEndpointChecker(endpoint))
		return;

	/* locally clear checker for all services that previously belonged to this endpoint */
	BOOST_FOREACH(const ConfigObject::Ptr& object, m_AllServices) {
		Service service = object;

		if (service.GetChecker() == endpoint->GetIdentity())
			service.SetChecker("");
	}

	/* remotely clear services for this endpoint */
	ClearServices(endpoint);
}

void DelegationComponent::DelegationTimerHandler(void)
{
	map<Endpoint::Ptr, int> histogram;

	EndpointManager::Iterator eit;
	for (eit = EndpointManager::GetInstance()->Begin(); eit != EndpointManager::GetInstance()->End(); eit++)
		histogram[eit->second] = 0;

	vector<Service> services;

	/* build "checker -> service count" histogram */
	BOOST_FOREACH(const ConfigObject::Ptr& object, m_AllServices) {
		Service service = object;

		services.push_back(service);

		string checker = service.GetChecker();
		if (checker.empty())
			continue;

		Endpoint::Ptr endpoint = EndpointManager::GetInstance()->GetEndpointByIdentity(checker);
		if (!endpoint)
			continue;

		histogram[endpoint]++;
	}

	std::random_shuffle(services.begin(), services.end());

	bool need_clear = false;
	int delegated = 0;

	/* re-assign services */
	BOOST_FOREACH(Service& service, services) {
		string checker = service.GetChecker();

		Endpoint::Ptr oldEndpoint;
		if (!checker.empty())
			oldEndpoint = EndpointManager::GetInstance()->GetEndpointByIdentity(checker);

		vector<Endpoint::Ptr> candidates = GetCheckerCandidates(service);

		int avg_services = 0, overflow_tolerance = 0;
		vector<Endpoint::Ptr>::iterator cit;

		if (candidates.size() > 0) {
			std::random_shuffle(candidates.begin(), candidates.end());

			stringstream msgbuf;
			msgbuf << "Service: " << service.GetName() << ", candidates: " << candidates.size();
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
		if (!checker.empty()) {
			need_clear = true;
			service.SetChecker("");

			if (oldEndpoint)
				histogram[oldEndpoint]--;
		}

		/* find a new checker for the service */
		BOOST_FOREACH(const Endpoint::Ptr& candidate, candidates) {
			/* does this checker already have too many services */
			if (histogram[candidate] > avg_services)
				continue;

			service.SetChecker(candidate->GetIdentity());
			histogram[candidate]++;

			delegated++;

			break;
		}

		assert(candidates.size() == 0 || !service.GetChecker().empty());
	}

	Endpoint::Ptr endpoint;
	int count;
	BOOST_FOREACH(tie(endpoint, count), histogram) {
		stringstream msgbuf;
		msgbuf << "histogram: " << endpoint->GetIdentity() << " - " << count;
		Logger::Write(LogInformation, "delegation", msgbuf.str());
	}

	if (delegated > 0) {
		if (need_clear) {
			Endpoint::Ptr endpoint;
			BOOST_FOREACH(tie(endpoint, tuples::ignore), histogram) {
				ClearServices(endpoint);
			}
		}

		BOOST_FOREACH(Service& service, services) {
			string checker = service.GetChecker();
			Endpoint::Ptr endpoint = EndpointManager::GetInstance()->GetEndpointByIdentity(checker);

			if (!endpoint)
				continue;

			AssignService(endpoint, service);
		}
	}

	stringstream msgbuf;
	msgbuf << "Updated delegations for " << delegated << " services";
	Logger::Write(LogInformation, "delegation", msgbuf.str());
}

EXPORT_COMPONENT(delegation, DelegationComponent);
