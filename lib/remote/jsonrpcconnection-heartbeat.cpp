/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/jsonrpcconnection.hpp"
#include "remote/messageorigin.hpp"
#include "remote/apifunction.hpp"
#include "base/initialize.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include <boost/asio/spawn.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/system/system_error.hpp>

using namespace icinga;

REGISTER_APIFUNCTION(Heartbeat, event, &JsonRpcConnection::HeartbeatAPIHandler);

/**
 * We still send a heartbeat without timeout here
 * to keep the m_Seen variable up to date. This is to keep the
 * cluster connection alive when there isn't much going on.
 */

void JsonRpcConnection::SetHeartbeatInterval(std::chrono::milliseconds interval)
{
	m_HeartbeatInterval = interval;
}

void JsonRpcConnection::HandleAndWriteHeartbeats(boost::asio::yield_context yc)
{
	boost::system::error_code ec;

	for (;;) {
		m_HeartbeatTimer.expires_after(m_HeartbeatInterval);
		m_HeartbeatTimer.async_wait(yc[ec]);

		if (m_ShuttingDown) {
			break;
		}
		
		SendMessageInternal(new Dictionary({
			{ "jsonrpc", "2.0" },
			{ "method", "event::Heartbeat" },
			{ "params", new Dictionary() }
		}));
	}
}

Value JsonRpcConnection::HeartbeatAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	return Empty;
}
