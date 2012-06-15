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

using namespace icinga;

string DelegationComponent::GetName(void) const
{
	return "delegation";
}

void DelegationComponent::Start(void)
{
	m_AllServices = boost::make_shared<ConfigObject::Set>(ConfigObject::GetAllObjects(), ConfigObject::MakeTypePredicate("service"));
	m_AllServices->OnObjectAdded.connect(boost::bind(&DelegationComponent::NewServiceHandler, this, _1));
	m_AllServices->OnObjectCommitted.connect(boost::bind(&DelegationComponent::NewServiceHandler, this, _1));
	m_AllServices->OnObjectRemoved.connect(boost::bind(&DelegationComponent::RemovedServiceHandler, this, _1));
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

void DelegationComponent::NewServiceHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea)
{
	AssignService(ea.Target);
}

void DelegationComponent::RemovedServiceHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea)
{
	RevokeService(ea.Target);
}

void DelegationComponent::AssignService(const ConfigObject::Ptr& service)
{
	RequestMessage request;
	request.SetMethod("checker::AssignService");

	MessagePart params;
	params.SetProperty("service", service->GetProperties());
	request.SetParams(params);

	Application::Log(LogInformation, "delegation", "Trying to delegate service '" + service->GetName() + "'");

	GetEndpointManager()->SendAPIMessage(m_DelegationEndpoint, request,
	    boost::bind(&DelegationComponent::AssignServiceResponseHandler, this, service, _1));
}

void DelegationComponent::AssignServiceResponseHandler(const ConfigObject::Ptr& service, const NewResponseEventArgs& nrea)
{
	if (nrea.TimedOut) {
		Application::Log(LogInformation, "delegation", "Service delegation for service '" + service->GetName() + "' timed out.");
	} else {
		service->SetTag("checker", nrea.Sender->GetIdentity());
		Application::Log(LogInformation, "delegation", "Service delegation for service '" + service->GetName() + "' was successful.");
	}
}

void DelegationComponent::RevokeService(const ConfigObject::Ptr& service)
{

}

void DelegationComponent::RevokeServiceResponseHandler(const NewResponseEventArgs& nrea)
{
}

void DelegationComponent::DelegationTimerHandler(void)
{
	ConfigObject::Set::Iterator it;
	for (it = m_AllServices->Begin(); it != m_AllServices->End(); it++) {
		ConfigObject::Ptr object = *it;

		string checker;
		if (object->GetTag("checker", &checker) && GetEndpointManager()->GetEndpointByIdentity(checker))
			continue;

		AssignService(object);
	}
}

EXPORT_COMPONENT(delegation, DelegationComponent);
