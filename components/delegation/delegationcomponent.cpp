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
	m_AllServices->OnObjectCommitted.connect(boost::bind(&DelegationComponent::ObjectCommittedHandler, this, _2));
	m_AllServices->Start();

	m_DelegationTimer = boost::make_shared<Timer>();
	m_DelegationTimer->SetInterval(30);
	m_DelegationTimer->OnTimerExpired.connect(boost::bind(&DelegationComponent::DelegationTimerHandler, this));
	m_DelegationTimer->Start();
	m_DelegationTimer->Reschedule(0);

	m_Endpoint = boost::make_shared<VirtualEndpoint>();
	m_Endpoint->RegisterPublication("checker::AssignService");
	m_Endpoint->RegisterPublication("checker::ClearServices");
	m_Endpoint->RegisterTopicHandler("checker::CheckResult",
	    boost::bind(&DelegationComponent::CheckResultRequestHandler, this, _2, _3));
	m_Endpoint->RegisterPublication("delegation::ServiceStatus");
	EndpointManager::GetInstance()->RegisterEndpoint(m_Endpoint);

	EndpointManager::GetInstance()->OnNewEndpoint.connect(bind(&DelegationComponent::NewEndpointHandler, this, _2));

	CIB::RequireInformation(CIB_Configuration);
}

void DelegationComponent::Stop(void)
{
	EndpointManager::Ptr mgr = EndpointManager::GetInstance();

	if (mgr)
		mgr->UnregisterEndpoint(m_Endpoint);
}

void DelegationComponent::ObjectCommittedHandler(const ConfigObject::Ptr& object)
{
	Service service(object);

	/* object was updated, clear its checker to make sure it's re-delegated by the delegation timer */
	service.SetChecker("");
}

void DelegationComponent::AssignService(const Endpoint::Ptr& checker, const Service& service)
{
	RequestMessage request;
	request.SetMethod("checker::AssignService");

	MessagePart params;
	params.SetProperty("service", service.GetConfigObject()->GetProperties());
	request.SetParams(params);

	Application::Log(LogDebug, "delegation", "Trying to delegate service '" + service.GetName() + "'");

	EndpointManager::GetInstance()->SendUnicastMessage(m_Endpoint, checker, request);
}

void DelegationComponent::ClearServices(const Endpoint::Ptr& checker)
{
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

	stringstream msgbuf;
	msgbuf << "Clearing assigned services for endpoint '" << endpoint->GetIdentity() << "'";
	Application::Log(LogInformation, "delegation", msgbuf.str());

	/* locally clear checker for all services that previously belonged to this endpoint */
	ConfigObject::Set::Iterator it;
	for (it = m_AllServices->Begin(); it != m_AllServices->End(); it++) {
		Service service = *it;

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
	ConfigObject::Set::Iterator it;
	for (it = m_AllServices->Begin(); it != m_AllServices->End(); it++) {
		Service service = *it;

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
	vector<Service>::iterator sit;
	for (sit = services.begin(); sit != services.end(); sit++) {
		Service service = *sit;

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
			Application::Log(LogDebug, "delegation", msgbuf.str());

			for (cit = candidates.begin(); cit != candidates.end(); cit++)
				avg_services += histogram[*cit];

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
		for (cit = candidates.begin(); cit != candidates.end(); cit++) {
			Endpoint::Ptr newEndpoint = *cit;

			/* does this checker already have too many services */
			if (histogram[newEndpoint] > avg_services)
				continue;

			service.SetChecker(newEndpoint->GetIdentity());
			histogram[newEndpoint]++;

			delegated++;

			break;
		}

		assert(candidates.size() == 0 || !service.GetChecker().empty());
	}

	map<Endpoint::Ptr, int>::iterator hit;
	for (hit = histogram.begin(); hit != histogram.end(); hit++) {
		stringstream msgbuf;
		msgbuf << "histogram: " << hit->first->GetIdentity() << " - " << hit->second;
		Application::Log(LogInformation, "delegation", msgbuf.str());
	}

	if (delegated > 0) {
		if (need_clear) {
			map<Endpoint::Ptr, int>::iterator hit;
			for (hit = histogram.begin(); hit != histogram.end(); hit++) {
				ClearServices(hit->first);
			}
		}

		for (sit = services.begin(); sit != services.end(); sit++) {
			string checker = sit->GetChecker();
			Endpoint::Ptr endpoint = EndpointManager::GetInstance()->GetEndpointByIdentity(checker);

			if (!endpoint)
				continue;

			AssignService(endpoint, *sit);
		}
	}

	stringstream msgbuf;
	msgbuf << "Updated delegations for " << delegated << " services";
	Application::Log(LogInformation, "delegation", msgbuf.str());
}

void DelegationComponent::CheckResultRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
		return;

	string svcname;
	if (!params.GetProperty("service", &svcname))
		return;

	Service service = Service::GetByName(svcname);

	/* validate that this is an authentic check result */
	if (!service.IsAllowedChecker(sender->GetIdentity()))
		return;

	/* TODO: send state update for dependant services */

	/* send state update */
	RequestMessage rm;
	rm.SetMethod("delegation::ServiceStatus");
	rm.SetParams(params);
	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint, rm);
}

EXPORT_COMPONENT(delegation, DelegationComponent);
