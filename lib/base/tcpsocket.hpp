/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include "base/i2-base.hpp"
#include "base/socket.hpp"
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>

namespace icinga
{

/**
 * A TCP socket. DEPRECATED - Use Boost ASIO instead.
 *
 * @ingroup base
 */
class TcpSocket final : public Socket
{
public:
	DECLARE_PTR_TYPEDEFS(TcpSocket);

	void Bind(const String& service, int family);
	void Bind(const String& node, const String& service, int family);

	void Connect(const String& node, const String& service);
};

typedef boost::asio::ip::tcp::socket::lowest_layer_type AsioTcpSocket;
typedef boost::asio::ip::tcp::resolver::results_type AsioDnsResponse;

AsioDnsResponse Resolve(const String& node, const String& service, boost::asio::yield_context* yc);

inline AsioDnsResponse Resolve(const String& node, const String& service)
{
	return Resolve(node, service, nullptr);
}

inline AsioDnsResponse Resolve(const String& node, const String& service, boost::asio::yield_context yc)
{
	return Resolve(node, service, &yc);
}

/**
 * TCP Connect based on Boost ASIO.
 *
 * @ingroup base
 */
void Connect(AsioTcpSocket& socket, const AsioDnsResponse& to, boost::asio::yield_context* yc);

inline void Connect(AsioTcpSocket& socket, const String& node, const String& service)
{
	Connect(socket, Resolve(node, service, nullptr), nullptr);
}

inline void Connect(AsioTcpSocket& socket, const String& node, const String& service, boost::asio::yield_context yc)
{
	Connect(socket, Resolve(node, service, &yc), &yc);
}

inline void Connect(AsioTcpSocket& socket, const AsioDnsResponse& to)
{
	Connect(socket, to, nullptr);
}

inline void Connect(AsioTcpSocket& socket, const AsioDnsResponse& to, boost::asio::yield_context yc)
{
	Connect(socket, to, &yc);
}

}

#endif /* TCPSOCKET_H */
