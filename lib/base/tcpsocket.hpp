/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TCPSOCKET_H
#define TCPSOCKET_H

#include "base/i2-base.hpp"
#include "base/io-engine.hpp"
#include "base/socket.hpp"
#include "base/logger.hpp"
#include <boost/asio/error.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/system/system_error.hpp>

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

/**
 * TCP Connect based on Boost ASIO.
 *
 * @ingroup base
 */
template<class Socket>
void Connect(Socket& socket, const String& node, const String& service)
{
	using boost::asio::ip::tcp;

	tcp::resolver resolver (IoEngine::Get().GetIoContext());
	tcp::resolver::query query (node, service);
	auto result (resolver.resolve(query));
	auto current (result.begin());

	for (;;) {
		try {
			socket.open(current->endpoint().protocol());
			socket.set_option(tcp::socket::keep_alive(true));
			socket.connect(current->endpoint());

			break;
		} catch (const std::exception& ex) {
			auto se (dynamic_cast<const boost::system::system_error*>(&ex));

			if (se && se->code() == boost::asio::error::operation_aborted || ++current == result.end()) {
				throw;
			}

			if (socket.is_open()) {
				socket.close();
			}
		}
	}
}

template<class Socket>
void Connect(Socket& socket, const String& node, const String& service, boost::asio::yield_context yc)
{
	using boost::asio::ip::tcp;

	tcp::resolver resolver (IoEngine::Get().GetIoContext());
	tcp::resolver::query query (node, service);
	auto result (resolver.async_resolve(query, yc));
	auto current (result.begin());

	for (;;) {
		try {
			Log(LogDebug, "Socket") << "Connecting to " << node << ":" << service << ": trying "
				<< current->endpoint().address().to_string() << ":" << current->endpoint().port();

			socket.open(current->endpoint().protocol());
			socket.set_option(tcp::socket::keep_alive(true));
			socket.async_connect(current->endpoint(), yc);

			Log(LogDebug, "Socket") << "Connecting to " << node << ":" << service << ": "
				<< current->endpoint().address().to_string() << ":" << current->endpoint().port() << " succeeded";

			break;
		} catch (const std::exception& ex) {
			Log(LogDebug, "Socket") << "Connecting to " << node << ":" << service << ": "
				<< current->endpoint().address().to_string() << ":" << current->endpoint().port() << " failed: " << ex.what();

			auto se (dynamic_cast<const boost::system::system_error*>(&ex));

			if (se && se->code() == boost::asio::error::operation_aborted || ++current == result.end()) {
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
