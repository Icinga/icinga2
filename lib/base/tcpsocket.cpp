/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/io-engine.hpp"
#include "base/tcpsocket.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include <boost/asio/error.hpp>
#include <boost/system/system_error.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <iostream>

using namespace icinga;

/**
 * Creates a socket and binds it to the specified service.
 *
 * @param service The service.
 * @param family The address family for the socket.
 */
void TcpSocket::Bind(const String& service, int family)
{
	Bind(String(), service, family);
}

/**
 * Creates a socket and binds it to the specified node and service.
 *
 * @param node The node.
 * @param service The service.
 * @param family The address family for the socket.
 */
void TcpSocket::Bind(const String& node, const String& service, int family)
{
	addrinfo hints;
	addrinfo *result;
	int error;
	const char *func;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	int rc = getaddrinfo(node.IsEmpty() ? nullptr : node.CStr(),
		service.CStr(), &hints, &result);

	if (rc != 0) {
		Log(LogCritical, "TcpSocket")
			<< "getaddrinfo() failed with error code " << rc << ", \"" << gai_strerror(rc) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("getaddrinfo")
			<< errinfo_getaddrinfo_error(rc));
	}

	int fd = INVALID_SOCKET;

	for (addrinfo *info = result; info != nullptr; info = info->ai_next) {
		fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		if (fd == INVALID_SOCKET) {
#ifdef _WIN32
			error = WSAGetLastError();
#else /* _WIN32 */
			error = errno;
#endif /* _WIN32 */
			func = "socket";

			continue;
		}

		const int optFalse = 0;
		setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char *>(&optFalse), sizeof(optFalse));

		const int optTrue = 1;
		setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&optTrue), sizeof(optTrue));
#ifdef SO_REUSEPORT
		setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char *>(&optTrue), sizeof(optTrue));
#endif /* SO_REUSEPORT */

		int rc = bind(fd, info->ai_addr, info->ai_addrlen);

		if (rc < 0) {
#ifdef _WIN32
			error = WSAGetLastError();
#else /* _WIN32 */
			error = errno;
#endif /* _WIN32 */
			func = "bind";

			closesocket(fd);

			continue;
		}

		SetFD(fd);

		break;
	}

	freeaddrinfo(result);

	if (GetFD() == INVALID_SOCKET) {
		Log(LogCritical, "TcpSocket")
			<< "Invalid socket: " << Utility::FormatErrorNumber(error);

#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function(func)
			<< boost::errinfo_errno(error));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function(func)
			<< errinfo_win32_error(error));
#endif /* _WIN32 */
	}
}

/**
 * Creates a socket and connects to the specified node and service.
 *
 * @param node The node.
 * @param service The service.
 */
void TcpSocket::Connect(const String& node, const String& service)
{
	addrinfo hints;
	addrinfo *result;
	int error;
	const char *func;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int rc = getaddrinfo(node.CStr(), service.CStr(), &hints, &result);

	if (rc != 0) {
		Log(LogCritical, "TcpSocket")
			<< "getaddrinfo() failed with error code " << rc << ", \"" << gai_strerror(rc) << "\"";

		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function("getaddrinfo")
			<< errinfo_getaddrinfo_error(rc));
	}

	SOCKET fd = INVALID_SOCKET;

	for (addrinfo *info = result; info != nullptr; info = info->ai_next) {
		fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		if (fd == INVALID_SOCKET) {
#ifdef _WIN32
			error = WSAGetLastError();
#else /* _WIN32 */
			error = errno;
#endif /* _WIN32 */
			func = "socket";

			continue;
		}

		const int optTrue = 1;
		if (setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, reinterpret_cast<const char *>(&optTrue), sizeof(optTrue)) != 0) {
#ifdef _WIN32
			error = WSAGetLastError();
#else /* _WIN32 */
			error = errno;
#endif /* _WIN32 */
			Log(LogWarning, "TcpSocket")
				<< "setsockopt() unable to enable TCP keep-alives with error code " << rc;
		}

		rc = connect(fd, info->ai_addr, info->ai_addrlen);

		if (rc < 0) {
#ifdef _WIN32
			error = WSAGetLastError();
#else /* _WIN32 */
			error = errno;
#endif /* _WIN32 */
			func = "connect";

			closesocket(fd);

			continue;
		}

		SetFD(fd);

		break;
	}

	freeaddrinfo(result);

	if (GetFD() == INVALID_SOCKET) {
		Log(LogCritical, "TcpSocket")
			<< "Invalid socket: " << Utility::FormatErrorNumber(error);

#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function(func)
			<< boost::errinfo_errno(error));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
			<< boost::errinfo_api_function(func)
			<< errinfo_win32_error(error));
#endif /* _WIN32 */
	}
}

using boost::asio::ip::tcp;

AsioDnsResponse icinga::Resolve(const String& node, const String& service, boost::asio::yield_context* yc)
{
	tcp::resolver resolver (IoEngine::Get().GetIoContext());
	tcp::resolver::query query (node, service);
	return yc ? resolver.async_resolve(query, *yc) : resolver.resolve(query);
}

void icinga::Connect(AsioTcpSocket& socket, const AsioDnsResponse& to, boost::asio::yield_context* yc)
{
	auto current (to.begin());

	for (;;) {
		try {
			socket.open(current->endpoint().protocol());
			socket.set_option(tcp::socket::keep_alive(true));

			if (yc) {
				socket.async_connect(current->endpoint(), *yc);
			} else {
				socket.connect(current->endpoint());
			}

			break;
		} catch (const std::exception& ex) {
			auto se (dynamic_cast<const boost::system::system_error*>(&ex));

			if (se && se->code() == boost::asio::error::operation_aborted || ++current == to.end()) {
				throw;
			}

			if (socket.is_open()) {
				socket.close();
			}
		}
	}
}
