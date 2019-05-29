/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef LIVESTATUSLISTENER_H
#define LIVESTATUSLISTENER_H

#include "livestatus/i2-livestatus.hpp"
#include "livestatus/livestatuslistener-ti.hpp"
#include "livestatus/livestatusquery.hpp"
#include "base/socket.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <boost/asio/generic/stream_protocol.hpp>
#include <thread>

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class LivestatusListener final : public ObjectImpl<LivestatusListener>
{
public:
	DECLARE_OBJECT(LivestatusListener);
	DECLARE_OBJECTNAME(LivestatusListener);

	typedef std::shared_ptr<boost::asio::generic::stream_protocol::socket> LivestatusSocket;

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static int GetClientsConnected();
	static int GetConnections();

	void ValidateSocketType(const Lazy<String>& lvalue, const ValidationUtils& utils) override;

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	void TcpServerThreadProc(const std::shared_ptr<boost::asio::ip::tcp::acceptor>& server);
	void UnixServerThreadProc(const std::shared_ptr<boost::asio::local::stream_protocol::acceptor>& server);

	void ClientHandler(const std::shared_ptr<boost::asio::generic::stream_protocol::socket>& socket);

	std::thread m_Thread;
};

}

#endif /* LIVESTATUSLISTENER_H */
