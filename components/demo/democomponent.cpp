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

#include "i2-demo.h"

using namespace icinga;

/**
 * Returns the name of the component.
 *
 * @returns The name.
 */
string DemoComponent::GetName(void) const
{
	return "demo";
}

/**
 * Starts the component.
 */
void DemoComponent::Start(void)
{
	m_Endpoint = boost::make_shared<VirtualEndpoint>();
	m_Endpoint->RegisterTopicHandler("demo::HelloWorld",
	    boost::bind(&DemoComponent::HelloWorldRequestHandler, this, _2, _3));
	m_Endpoint->RegisterPublication("demo::HelloWorld");
	EndpointManager::GetInstance()->RegisterEndpoint(m_Endpoint);

	m_DemoTimer = boost::make_shared<Timer>();
	m_DemoTimer->SetInterval(5);
	m_DemoTimer->OnTimerExpired.connect(boost::bind(&DemoComponent::DemoTimerHandler, this));
	m_DemoTimer->Start();
}

/**
 * Stops the component.
 */
void DemoComponent::Stop(void)
{
	EndpointManager::Ptr endpointManager = EndpointManager::GetInstance();

	if (endpointManager)
		endpointManager->UnregisterEndpoint(m_Endpoint);
}

/**
 * Periodically sends a demo::HelloWorld message.
 *
 * @param - Event arguments for the timer.
 */
void DemoComponent::DemoTimerHandler(void)
{
	Application::Log(LogInformation, "demo", "Sending multicast 'hello world' message.");

	RequestMessage request;
	request.SetMethod("demo::HelloWorld");

	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint, request);
}

/**
 * Processes demo::HelloWorld messages.
 */
void DemoComponent::HelloWorldRequestHandler(const Endpoint::Ptr& sender, const RequestMessage& request)
{
	Application::Log(LogInformation, "demo", "Got 'hello world' from address=" + sender->GetAddress() + ", identity=" + sender->GetIdentity());
}

EXPORT_COMPONENT(demo, DemoComponent);
