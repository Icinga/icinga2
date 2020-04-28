/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef TLSSTREAM_H
#define TLSSTREAM_H

#include "base/i2-base.hpp"
#include "base/shared.hpp"
#include "base/socket.hpp"
#include "base/stream.hpp"
#include "base/tlsutility.hpp"
#include "base/fifo.hpp"
#include <ios>
#include <memory>
#include <utility>
#include <boost/asio/buffer.hpp>
#include <boost/asio/buffered_stream.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/char_traits.hpp>
#include <boost/iostreams/device/array.hpp>

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

class OneCharOrBlockSource
{
public:
	typedef char char_type;
	typedef boost::iostreams::array::category category;

	inline OneCharOrBlockSource() : m_Char(boost::iostreams::WOULD_BLOCK)
	{
	}

	OneCharOrBlockSource(const OneCharOrBlockSource&) = delete;
	OneCharOrBlockSource(OneCharOrBlockSource&&) = delete;
	OneCharOrBlockSource& operator=(const OneCharOrBlockSource&) = delete;
	OneCharOrBlockSource& operator=(OneCharOrBlockSource&&) = delete;

	inline boost::iostreams::char_traits<char>::int_type GetChar()
	{
		return m_Char;
	}

	inline void SetChar(boost::iostreams::char_traits<char>::int_type c)
	{
		m_Char = c;
	}

	std::streamsize read(char *buf, std::streamsize bufSize);

private:
	boost::iostreams::char_traits<char>::int_type m_Char;
};

template<class AWS>
class GzipStream : public AWS
{
public:
	using AWS::AWS;

	auto async_write_some(boost::asio::const_buffer cb, boost::asio::yield_context yc) -> decltype(((AWS*)nullptr)->async_write_some(cb, yc))
	{
		using ct = boost::iostreams::char_traits<char>;

		if (!cb.size()) {
			return 0u;
		}

		OneCharOrBlockSource ocobs;
		ocobs.SetChar(ct::to_int_type(*(char*)cb.data()));

		for (;;) {
			{
				char buf;

				if (m_Compressor.read(ocobs, &buf, 1) > 0u) {
					((AWS*)this)->async_write_some(boost::asio::const_buffer(&buf, 1), yc);
				}
			}

			if (!ct::is_good(ocobs.GetChar())) {
				return 1u;
			}
		}
	}

	template<class... Args>
	auto async_write_some(Args&&... args) -> decltype(((AWS*)nullptr)->async_write_some(std::forward<Args>(args)...)) = delete;

private:
	boost::iostreams::gzip_compressor m_Compressor;
};

template<class ARS>
class GunzipStream : public ARS
{
public:
	using ARS::ARS;

	auto async_read_some(boost::asio::mutable_buffer mb, boost::asio::yield_context yc) -> decltype(((ARS*)nullptr)->async_read_some(mb, yc))
	{
		namespace asio = boost::asio;
		namespace ios = boost::iostreams;

		if (!mb.size()) {
			return 0u;
		}

		OneCharOrBlockSource ocobs;

		{
			auto count (m_Decompressor.read(ocobs, (char*)mb.data(), mb.size()));

			if (count > 0u) {
				return count;
			}
		}

		for (;;) {
			{
				char buf;

				try {
					((ARS*)this)->async_read_some(asio::mutable_buffer(&buf, 1), yc);
				} catch (const boost::system::system_error& se) {
					if (se.code() == asio::error::eof) {
						ocobs.SetChar(EOF);

						auto count (m_Decompressor.read(ocobs, (char*)mb.data(), mb.size()));

						if (count > 0u) {
							return count;
						}
					}

					throw;
				}

				ocobs.SetChar(ios::char_traits<char>::to_int_type(buf));
			}

			auto count (m_Decompressor.read(ocobs, (char*)mb.data(), mb.size()));

			if (count > 0u) {
				return count;
			}
		}
	}

	template<class... Args>
	auto async_read_some(Args&&... args) -> decltype(((ARS*)nullptr)->async_read_some(std::forward<Args>(args)...)) = delete;

private:
	boost::iostreams::gzip_decompressor m_Decompressor;
};

}

#endif /* TLSSTREAM_H */
