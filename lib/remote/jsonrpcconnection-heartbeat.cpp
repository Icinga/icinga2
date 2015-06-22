/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "remote/jsonrpcconnection.hpp"
#include "remote/messageorigin.hpp"
#include "remote/apifunction.hpp"
#include "base/initialize.hpp"
#include "base/dynamictype.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_APIFUNCTION(Heartbeat, event, &JsonRpcConnection::HeartbeatAPIHandler);

static Timer::Ptr l_HeartbeatTimer;

static void StartHeartbeatTimer(void)
{
	l_HeartbeatTimer = new Timer();
	l_HeartbeatTimer->OnTimerExpired.connect(boost::bind(&JsonRpcConnection::HeartbeatTimerHandler));
	l_HeartbeatTimer->SetInterval(10);
	l_HeartbeatTimer->Start();
}

INITIALIZE_ONCE(StartHeartbeatTimer);

void JsonRpcConnection::HeartbeatTimerHandler(void)
{
	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjectsByType<Endpoint>()) {
		BOOST_FOREACH(const JsonRpcConnection::Ptr& client, endpoint->GetClients()) {
			if (endpoint->GetSyncing()) {
				Log(LogInformation, "JsonRpcConnection")
				    << "Not sending heartbeat for endpoint '" << endpoint->GetName() << "' because we're replaying the log for it.";
				continue;
			}

			if (client->m_NextHeartbeat != 0 && client->m_NextHeartbeat < Utility::GetTime()) {
				Log(LogWarning, "JsonRpcConnection")
				    << "Client for endpoint '" << endpoint->GetName() << "' has requested "
				    << "heartbeat message but hasn't responded in time. Closing connection.";

				client->Disconnect();
				continue;
			}

			Dictionary::Ptr request = new Dictionary();
			request->Set("jsonrpc", "2.0");
			request->Set("method", "event::Heartbeat");

			Dictionary::Ptr params = new Dictionary();
			params->Set("timeout", 120);

			request->Set("params", params);

			client->SendMessage(request);
		}
	}
}

Value JsonRpcConnection::HeartbeatAPIHandler(const MessageOrigin& origin, const Dictionary::Ptr& params)
{
	Value vtimeout = params->Get("timeout");

	if (!vtimeout.IsEmpty()) {
		origin.FromClient->m_NextHeartbeat = Utility::GetTime() + vtimeout;
		origin.FromClient->m_HeartbeatTimeout = vtimeout;
	}

	return Empty;
}

