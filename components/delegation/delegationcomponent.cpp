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
	m_AllServices->OnObjectAdded += bind_weak(&DelegationComponent::NewServiceHandler, shared_from_this());
	m_AllServices->OnObjectCommitted += bind_weak(&DelegationComponent::NewServiceHandler, shared_from_this());
	m_AllServices->OnObjectRemoved += bind_weak(&DelegationComponent::RemovedServiceHandler, shared_from_this());
	m_AllServices->Start();

	m_DelegationEndpoint = make_shared<VirtualEndpoint>();
	m_DelegationEndpoint->RegisterPublication("checker::AssignService");
	m_DelegationEndpoint->RegisterPublication("checker::RevokeService");
	GetEndpointManager()->RegisterEndpoint(m_DelegationEndpoint);

	RequestMessage rm;
	rm.SetMethod("checker::AssignService");
	GetEndpointManager()->SendAPIMessage(m_DelegationEndpoint, rm, bind(&DelegationComponent::TestResponseHandler, this, _1));
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

	GetEndpointManager()->SendAPIMessage(m_DelegationEndpoint, request,
	    bind_weak(&DelegationComponent::AssignServiceResponseHandler, shared_from_this()));
}

int DelegationComponent::AssignServiceResponseHandler(const NewResponseEventArgs& nrea)
{
	return 0;
}

void DelegationComponent::RevokeService(const ConfigObject::Ptr& service)
{

}

int DelegationComponent::RevokeServiceResponseHandler(const NewResponseEventArgs& nrea)
{
	return 0;
}

int DelegationComponent::TestResponseHandler(const NewResponseEventArgs& ea)
{
	Application::Log("Response handler called.");

	return 0;
}

EXPORT_COMPONENT(delegation, DelegationComponent);
