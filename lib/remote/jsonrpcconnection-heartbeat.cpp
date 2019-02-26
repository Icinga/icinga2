/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/jsonrpcconnection.hpp"
#include "remote/messageorigin.hpp"
#include "remote/apifunction.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>

using namespace icinga;

REGISTER_APIFUNCTION(Heartbeat, event, &JsonRpcConnection::HeartbeatAPIHandler);

void JsonRpcConnection::HandleAndWriteHeartbeats(boost::asio::yield_context yc)
{
	boost::asio::deadline_timer timer (m_Stream->get_io_service());

	for (;;) {
		timer.expires_from_now(boost::posix_time::seconds(10));
		timer.async_wait(yc);

		if (m_ShuttingDown) {
			break;
		}

		if (m_NextHeartbeat != 0 && m_NextHeartbeat < Utility::GetTime()) {
			Log(LogWarning, "JsonRpcConnection")
				<< "Client for endpoint '" << m_Endpoint->GetName() << "' has requested "
				<< "heartbeat message but hasn't responded in time. Closing connection.";

			Disconnect();
			break;
		}

		SendMessageInternal(new Dictionary({
			{ "jsonrpc", "2.0" },
			{ "method", "event::Heartbeat" },
			{ "params", new Dictionary({
				{ "timeout", 120 }
			}) }
		}));
	}
}

Value JsonRpcConnection::HeartbeatAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	Value vtimeout = params->Get("timeout");

	if (!vtimeout.IsEmpty()) {
		origin->FromClient->m_NextHeartbeat = Utility::GetTime() + vtimeout;
	}

	return Empty;
}

