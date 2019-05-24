/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TLSSTREAM_H
#define TLSSTREAM_H

#include "base/i2-base.hpp"
#include "base/socket.hpp"
#include "base/socketevents.hpp"
#include "base/stream.hpp"
#include "base/tlsutility.hpp"
#include "base/fifo.hpp"
#include <memory>
#include <utility>
#include <boost/asio/buffered_stream.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace icinga
{

struct UnbufferedAsioTlsStreamParams
{
	boost::asio::io_service& IoService;
	boost::asio::ssl::context& SslContext;
	const String& Hostname;
};

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> AsioTcpTlsStream;

class UnbufferedAsioTlsStream : public AsioTcpTlsStream
{
public:
	inline
	UnbufferedAsioTlsStream(UnbufferedAsioTlsStreamParams& init)
		: stream(init.IoService, init.SslContext), m_VerifyOK(true), m_Hostname(init.Hostname)
	{
	}

	bool IsVerifyOK() const;
	String GetVerifyError() const;
	std::shared_ptr<X509> GetPeerCertificate();

	template<class... Args>
	inline
	auto async_handshake(handshake_type type, Args&&... args) -> decltype(((AsioTcpTlsStream*)nullptr)->async_handshake(type, std::forward<Args>(args)...))
	{
		BeforeHandshake(type);

		return AsioTcpTlsStream::async_handshake(type, std::forward<Args>(args)...);
	}

	template<class... Args>
	inline
	auto handshake(handshake_type type, Args&&... args) -> decltype(((AsioTcpTlsStream*)nullptr)->handshake(type, std::forward<Args>(args)...))
	{
		BeforeHandshake(type);

		return AsioTcpTlsStream::handshake(type, std::forward<Args>(args)...);
	}

private:
	bool m_VerifyOK;
	String m_VerifyError;
	String m_Hostname;

	void BeforeHandshake(handshake_type type);
};

class AsioTlsStream : public boost::asio::buffered_stream<UnbufferedAsioTlsStream>
{
public:
	inline
	AsioTlsStream(boost::asio::io_service& ioService, boost::asio::ssl::context& sslContext, const String& hostname = String())
		: AsioTlsStream(UnbufferedAsioTlsStreamParams{ioService, sslContext, hostname})
	{
	}

private:
	inline
	AsioTlsStream(UnbufferedAsioTlsStreamParams init)
		: buffered_stream(init)
	{
	}
};

typedef boost::asio::buffered_stream<boost::asio::ip::tcp::socket> AsioTcpStream;
typedef std::pair<std::shared_ptr<AsioTlsStream>, std::shared_ptr<AsioTcpStream>> OptionalTlsStream;

}

#endif /* TLSSTREAM_H */
