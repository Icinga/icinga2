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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#include "i2-demo.h"

using namespace icinga;

/**
 * GetName
 *
 * Returns the name of the component.
 *
 * @returns The name.
 */
string DemoComponent::GetName(void) const
{
	return "democomponent";
}

/**
 * Start
 *
 * Starts the component.
 */
void DemoComponent::Start(void)
{
	m_DemoEndpoint = make_shared<VirtualEndpoint>();
	m_DemoEndpoint->RegisterMethodHandler("demo::HelloWorld",
	    bind_weak(&DemoComponent::HelloWorldRequestHandler, shared_from_this()));
	m_DemoEndpoint->RegisterMethodSource("demo::HelloWorld");

	EndpointManager::Ptr endpointManager = GetIcingaApplication()->GetEndpointManager();
	endpointManager->RegisterEndpoint(m_DemoEndpoint);

	m_DemoTimer = make_shared<Timer>();
	m_DemoTimer->SetInterval(5);
	m_DemoTimer->OnTimerExpired += bind_weak(&DemoComponent::DemoTimerHandler, shared_from_this());
	m_DemoTimer->Start();
}

/**
 * Stop
 *
 * Stops the component.
 */
void DemoComponent::Stop(void)
{
	IcingaApplication::Ptr app = GetIcingaApplication();

	if (app) {
		EndpointManager::Ptr endpointManager = app->GetEndpointManager();
		endpointManager->UnregisterEndpoint(m_DemoEndpoint);
	}
}

/**
 * DemoTimerHandler
 *
 * Periodically sends a demo::HelloWorld message.
 *
 * @param - Event arguments for the timer.
 * @returns 0
 */
int DemoComponent::DemoTimerHandler(const TimerEventArgs&)
{
	Application::Log("Sending multicast 'hello world' message.");

	JsonRpcRequest request;
	request.SetMethod("demo::HelloWorld");

	EndpointManager::Ptr endpointManager = GetIcingaApplication()->GetEndpointManager();
	endpointManager->SendMulticastRequest(m_DemoEndpoint, request);

	return 0;
}

/**
 * HelloWorldRequestHandler
 *
 * Processes demo::HelloWorld messages.
 */
int DemoComponent::HelloWorldRequestHandler(const NewRequestEventArgs& nrea)
{
	Application::Log("Got 'hello world' from address=" + nrea.Sender->GetAddress() + ", identity=" + nrea.Sender->GetIdentity());

	return 0;
}

EXPORT_COMPONENT(DemoComponent);
