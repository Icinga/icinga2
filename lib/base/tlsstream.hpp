/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TLSSTREAM_H
#define TLSSTREAM_H

#include "base/i2-base.hpp"
#include "base/shared.hpp"
#include "base/socket.hpp"
#include "base/stream.hpp"
#include "base/tlsutility.hpp"
#include "base/fifo.hpp"
#include <memory>
#include <utility>
#include <boost/asio/buffered_stream.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>

namespace icinga
{

struct UnbufferedAsioTlsStreamParams
{
	boost::asio::io_context& IoContext;
	boost::asio::ssl::context& SslContext;
	const String& Hostname;
};

typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> AsioTcpTlsStream;

class UnbufferedAsioTlsStream : public AsioTcpTlsStream
{
public:
	inline
	UnbufferedAsioTlsStream(UnbufferedAsioTlsStreamParams& init)
		: stream(init.IoContext, init.SslContext), m_VerifyOK(true), m_Hostname(init.Hostname)
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
	AsioTlsStream(boost::asio::io_context& ioContext, boost::asio::ssl::context& sslContext, const String& hostname = String())
		: AsioTlsStream(UnbufferedAsioTlsStreamParams{ioContext, sslContext, hostname})
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
typedef std::pair<Shared<AsioTlsStream>::Ptr, Shared<AsioTcpStream>::Ptr> OptionalTlsStream;

template<class Stream>
class AsyncSharedStream : public Shared<Stream>::Ptr
{
public:
	using Shared<Stream>::Ptr::Ptr;

	typedef typename Stream::executor_type executor_type;

	auto get_executor() -> decltype((*(typename Shared<Stream>::Ptr*)this)->get_executor())
	{
		return (*(typename Shared<Stream>::Ptr*)this)->get_executor();
	}

	template<class... Args>
	auto async_read_some(Args&&... args) -> decltype((*(typename Shared<Stream>::Ptr*)this)->async_read_some(std::forward<Args>(args)...))
	{
		return (*(typename Shared<Stream>::Ptr*)this)->async_read_some(std::forward<Args>(args)...);
	}

	template<class... Args>
	auto async_write_some(Args&&... args) -> decltype((*(typename Shared<Stream>::Ptr*)this)->async_write_some(std::forward<Args>(args)...))
	{
		return (*(typename Shared<Stream>::Ptr*)this)->async_write_some(std::forward<Args>(args)...);
	}
};

}

#endif /* TLSSTREAM_H */
