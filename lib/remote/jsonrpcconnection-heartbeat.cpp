/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/jsonrpcconnection.hpp"
#include "remote/messageorigin.hpp"
#include "remote/apifunction.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"

using namespace icinga;

REGISTER_APIFUNCTION(Heartbeat, event, &JsonRpcConnection::HeartbeatAPIHandler);

void JsonRpcConnection::HeartbeatTimerHandler()
{
	for (const Endpoint::Ptr& endpoint : ConfigType::GetObjectsByType<Endpoint>()) {
		for (const JsonRpcConnection::Ptr& client : endpoint->GetClients()) {
			if (client->m_NextHeartbeat != 0 && client->m_NextHeartbeat < Utility::GetTime()) {
				Log(LogWarning, "JsonRpcConnection")
					<< "Client for endpoint '" << endpoint->GetName() << "' has requested "
					<< "heartbeat message but hasn't responded in time. Closing connection.";

				client->Disconnect();
				continue;
			}

			Dictionary::Ptr request = new Dictionary({
				{ "jsonrpc", "2.0" },
				{ "method", "event::Heartbeat" },
				{ "params", new Dictionary({
					{ "timeout", 120 }
				}) }
			});

			client->SendMessage(request);
		}
	}
}

Value JsonRpcConnection::HeartbeatAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Value vtimeout = params->Get("timeout");

	if (!vtimeout.IsEmpty()) {
		origin->FromClient->m_NextHeartbeat = Utility::GetTime() + vtimeout;
		origin->FromClient->m_HeartbeatTimeout = vtimeout;
	}

	return Empty;
}

