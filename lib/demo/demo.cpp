/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "demo/demo.hpp"
#include "demo/demo.tcpp"
#include "remote/apilistener.hpp"
#include "remote/apifunction.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_TYPE(Demo);

REGISTER_APIFUNCTION(HelloWorld, demo, &Demo::DemoMessageHandler);

/**
 * Starts the component.
 */
void Demo::Start(bool runtimeCreated)
{
	ObjectImpl<Demo>::Start(runtimeCreated);

	m_DemoTimer = new Timer();
	m_DemoTimer->SetInterval(5);
	m_DemoTimer->OnTimerExpired.connect(std::bind(&Demo::DemoTimerHandler, this));
	m_DemoTimer->Start();
}

/**
 * Periodically broadcasts an API message.
 */
void Demo::DemoTimerHandler(void)
{
	Dictionary::Ptr message = new Dictionary();
	message->Set("method", "demo::HelloWorld");

	ApiListener::Ptr listener = ApiListener::GetInstance();
	if (listener) {
		MessageOrigin::Ptr origin = new MessageOrigin();
		listener->RelayMessage(origin, nullptr, message, true);
		Log(LogInformation, "Demo", "Sent demo::HelloWorld message");
	}
}

Value Demo::DemoMessageHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr&)
{
	Log(LogInformation, "Demo")
	    << "Got demo message from '" << origin->FromClient->GetEndpoint()->GetName() << "'";

	return Empty;
}
