/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "remote/apiuser.hpp"
#include "base/string.hpp"
#include "base/tlsstream.hpp"
#include <memory>
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/spawn.hpp>

namespace icinga
{

/**
 * An API client connection.
 *
 * @ingroup remote
 */
class HttpServerConnection final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(HttpServerConnection);

	HttpServerConnection(const String& identity, bool authenticated, const Shared<AsioTlsStream>::Ptr& stream);

	void Start();
	void Disconnect();
	void StartStreaming();

	bool Disconnected();

private:
	ApiUser::Ptr m_ApiUser;
	Shared<AsioTlsStream>::Ptr m_Stream;
	double m_Seen;
	String m_PeerAddress;
	boost::asio::io_context::strand m_IoStrand;
	bool m_ShuttingDown;
	bool m_HasStartedStreaming;
	boost::asio::deadline_timer m_CheckLivenessTimer;

	HttpServerConnection(const String& identity, bool authenticated, const Shared<AsioTlsStream>::Ptr& stream, boost::asio::io_context& io);

	void ProcessMessages(boost::asio::yield_context yc);
	void CheckLiveness(boost::asio::yield_context yc);
};

}
