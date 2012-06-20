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
	m_AllServices->OnObjectAdded.connect(boost::bind(&DelegationComponent::NewServiceHandler, this, _2));
	m_AllServices->OnObjectCommitted.connect(boost::bind(&DelegationComponent::NewServiceHandler, this, _2));
	m_AllServices->OnObjectRemoved.connect(boost::bind(&DelegationComponent::RemovedServiceHandler, this, _2));
	m_AllServices->Start();

	m_DelegationTimer = boost::make_shared<Timer>();
	m_DelegationTimer->SetInterval(30);
	m_DelegationTimer->OnTimerExpired.connect(boost::bind(&DelegationComponent::DelegationTimerHandler, this));
	m_DelegationTimer->Start();

	m_DelegationEndpoint = boost::make_shared<VirtualEndpoint>();
	m_DelegationEndpoint->RegisterPublication("checker::AssignService");
	m_DelegationEndpoint->RegisterPublication("checker::RevokeService");
	GetEndpointManager()->RegisterEndpoint(m_DelegationEndpoint);
}

void DelegationComponent::Stop(void)
{
	EndpointManager::Ptr mgr = GetEndpointManager();

	if (mgr)
		mgr->UnregisterEndpoint(m_DelegationEndpoint);
}

void DelegationComponent::NewServiceHandler(const Service& object)
{
	AssignService(object);
}

void DelegationComponent::RemovedServiceHandler(const Service& object)
{
	RevokeService(object);
}

void DelegationComponent::AssignService(const Service& service)
{
	RequestMessage request;
	request.SetMethod("checker::AssignService");

	MessagePart params;
	params.SetProperty("service", service.GetConfigObject()->GetProperties());
	request.SetParams(params);

	Application::Log(LogDebug, "delegation", "Trying to delegate service '" + service.GetName() + "'");

	GetEndpointManager()->SendAPIMessage(m_DelegationEndpoint, request,
	    boost::bind(&DelegationComponent::AssignServiceResponseHandler, this, service, _2, _5));
}

void DelegationComponent::AssignServiceResponseHandler(Service& service, const Endpoint::Ptr& sender, bool timedOut)
{
	if (timedOut) {
		Application::Log(LogDebug, "delegation", "Service delegation for service '" + service.GetName() + "' timed out.");
	} else {
		service.SetChecker(sender->GetIdentity());
		Application::Log(LogDebug, "delegation", "Service delegation for service '" + service.GetName() + "' was successful.");
	}
}

void DelegationComponent::RevokeService(const Service& service)
{

}

void DelegationComponent::RevokeServiceResponseHandler(Service& service, const Endpoint::Ptr& sender, bool timedOut)
{
}

vector<Endpoint::Ptr> DelegationComponent::GetCheckerCandidates(const Service& service) const
{
	vector<Endpoint::Ptr> candidates;

	EndpointManager::Iterator it;
	for (it = GetEndpointManager()->Begin(); it != GetEndpointManager()->End(); it++)
		candidates.push_back(it->second);

	return candidates;
}

void DelegationComponent::DelegationTimerHandler(void)
{
	map<Endpoint::Ptr, int> histogram;

	EndpointManager::Iterator eit;
	for (eit = GetEndpointManager()->Begin(); eit != GetEndpointManager()->End(); eit++) {
		histogram[eit->second] = 0;
	}

	/* nothing to do if we have no checkers */
	if (histogram.size() == 0)
		return;

	vector<Service> services;

	/* build "checker -> service count" histogram */
	ConfigObject::Set::Iterator it;
	for (it = m_AllServices->Begin(); it != m_AllServices->End(); it++) {
		Service service = *it;

		services.push_back(service);

		string checker = service.GetChecker();
		if (checker.empty())
			continue;

		Endpoint::Ptr endpoint = GetEndpointManager()->GetEndpointByIdentity(checker);
		if (!endpoint)
			continue;

		histogram[endpoint]++;
	}

	std::random_shuffle(services.begin(), services.end());

	long delegated = 0;

	/* re-assign services */
	vector<Service>::iterator sit;
	for (sit = services.begin(); sit != services.end(); it++) {
		Service service = *sit;

		string checker = service.GetChecker();

		Endpoint::Ptr oldEndpoint;
		if (!checker.empty())
			oldEndpoint = GetEndpointManager()->GetEndpointByIdentity(checker);

		vector<Endpoint::Ptr> candidates = GetCheckerCandidates(service);

		long avg_services = 0;
		vector<Endpoint::Ptr>::iterator cit;
		for (cit = candidates.begin(); cit != candidates.end(); cit++)
			avg_services += histogram[*cit];

		avg_services /= candidates.size();
		long overflow_tolerance = candidates.size() * 2;

		/* don't re-assign service if the checker is still valid
		 * and doesn't have too many services */
		if (oldEndpoint && find(candidates.begin(), candidates.end(), oldEndpoint) != candidates.end() &&
		    histogram[oldEndpoint] <= avg_services + overflow_tolerance)
			continue;

		/* clear the service's current checker */
		if (!checker.empty()) {
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
		}

		assert(!service.GetChecker().empty());
	}

	stringstream msgbuf;
	msgbuf << "Re-delegated " << delegated << " services";
	Application::Log(LogInformation, "delegation", msgbuf.str());
}

EXPORT_COMPONENT(delegation, DelegationComponent);
