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

REGISTER_TYPE(DemoComponent);

DemoComponent::DemoComponent(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{ }

/**
 * Starts the component.
 */
void DemoComponent::Start(void)
{
	m_Endpoint = Endpoint::MakeEndpoint("demo", false);
	m_Endpoint->RegisterTopicHandler("demo::HelloWorld",
	    boost::bind(&DemoComponent::HelloWorldRequestHandler, this, _2,
	    _3));

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
	m_Endpoint->Unregister();
}

/**
 * Periodically sends a demo::HelloWorld message.
 *
 * @param - Event arguments for the timer.
 */
void DemoComponent::DemoTimerHandler(void)
{
	Logger::Write(LogInformation, "demo", "Sending multicast 'hello world' message.");

	RequestMessage request;
	request.SetMethod("demo::HelloWorld");

	EndpointManager::GetInstance()->SendMulticastMessage(m_Endpoint, request);
}

/**
 * Processes demo::HelloWorld messages.
 */
void DemoComponent::HelloWorldRequestHandler(const Endpoint::Ptr& sender,
    const RequestMessage&)
{
	Logger::Write(LogInformation, "demo", "Got 'hello world' from identity=" +
	    (sender ? sender->GetName() : "(anonymous)"));
}
