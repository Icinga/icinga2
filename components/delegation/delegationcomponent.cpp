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
	m_AllServices = make_shared<ConfigObject::Set>(ConfigObject::GetAllObjects(), ConfigObject::MakeTypePredicate("service"));
	m_AllServices->OnObjectAdded.connect(bind(&DelegationComponent::NewServiceHandler, this, _1));
	m_AllServices->OnObjectCommitted.connect(bind(&DelegationComponent::NewServiceHandler, this, _1));
	m_AllServices->OnObjectRemoved.connect(bind(&DelegationComponent::RemovedServiceHandler, this, _1));
	m_AllServices->Start();

	m_DelegationTimer = make_shared<Timer>();
	m_DelegationTimer->SetInterval(30);
	m_DelegationTimer->OnTimerExpired.connect(bind(&DelegationComponent::DelegationTimerHandler, this, _1));
	m_DelegationTimer->Start();

	m_DelegationEndpoint = make_shared<VirtualEndpoint>();
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

int DelegationComponent::NewServiceHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea)
{
	AssignService(ea.Target);
	return 0;
}

int DelegationComponent::RemovedServiceHandler(const ObjectSetEventArgs<ConfigObject::Ptr>& ea)
{
	RevokeService(ea.Target);
	return 0;
}

void DelegationComponent::AssignService(const ConfigObject::Ptr& service)
{
	RequestMessage request;
	request.SetMethod("checker::AssignService");

	MessagePart params;
	params.SetProperty("service", service->GetProperties());
	request.SetParams(params);

	Application::Log("Trying to delegate service '" + service->GetName() + "'");

	GetEndpointManager()->SendAPIMessage(m_DelegationEndpoint, request,
	    bind(&DelegationComponent::AssignServiceResponseHandler, this, service, _1));
}

int DelegationComponent::AssignServiceResponseHandler(const ConfigObject::Ptr& service, const NewResponseEventArgs& nrea)
{
	if (nrea.TimedOut)
		Application::Log("Service delegation for service '" + service->GetName() + "' timed out.");
	else
		Application::Log("Service delegation for service '" + service->GetName() + "'was successful.");

	return 0;
}

void DelegationComponent::RevokeService(const ConfigObject::Ptr& service)
{

}

int DelegationComponent::RevokeServiceResponseHandler(const NewResponseEventArgs& nrea)
{
	return 0;
}

int DelegationComponent::DelegationTimerHandler(const TimerEventArgs& ea)
{
	ConfigObject::Set::Iterator it;
	for (it = m_AllServices->Begin(); it != m_AllServices->End(); it++) {
		ConfigObject::Ptr object = *it;

		string checker;
		if (object->GetTag("checker", &checker) && GetEndpointManager()->GetEndpointByIdentity(checker))
			continue;

		AssignService(object);
	}

	return 0;
}

int DelegationComponent::TestResponseHandler(const NewResponseEventArgs& ea)
{
	Application::Log("Response handler called.");

	return 0;
}

EXPORT_COMPONENT(delegation, DelegationComponent);
