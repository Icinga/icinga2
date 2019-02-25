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
 * A TCP socket.
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

template<class Socket>
void Connect(Socket& socket, const String& node, const String& service, boost::asio::yield_context yc)
{
	using boost::asio::ip::tcp;

	tcp::resolver resolver (socket.get_io_service());
	tcp::resolver::query query (node, service);
	auto result (resolver.async_resolve(query, yc));
	auto current (result.begin());

	for (;;) {
		try {
			socket.open(current->endpoint().protocol());
			socket.set_option(tcp::socket::keep_alive(true));
			socket.async_connect(current->endpoint(), yc);

			break;
		} catch (const std::exception&) {
			if (++current == result.end()) {
				throw;
			}

			if (socket.is_open()) {
				socket.close();
			}
		}
	}
}

}

#endif /* TCPSOCKET_H */
