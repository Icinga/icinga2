/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef NETSTRING_H
#define NETSTRING_H

#include "base/i2-base.hpp"
#include "base/stream.hpp"
#include "base/tlsstream.hpp"
#include <cstdint>
#include <memory>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>
#include <sstream>

namespace icinga
{

class String;

/**
 * Helper functions for reading/writing messages in the netstring format.
 *
 * @see https://cr.yp.to/proto/netstrings.txt
 *
 * @ingroup base
 */
class NetString
{
public:
	static StreamReadStatus ReadStringFromStream(const Stream::Ptr& stream, String *message, StreamReadContext& context,
		bool may_wait = false, ssize_t maxMessageLength = -1);
	static String ReadStringFromStream(const Shared<AsioTlsStream>::Ptr& stream, ssize_t maxMessageLength = -1);

	/**
	 * Reads data from a stream in netstring format.
	 *
	 * @param stream The stream to read from.
	 * @returns The String that has been read from the IOQueue.
	 * @exception invalid_argument The input stream is invalid.
	 * @see https://github.com/PeterScott/netstring-c/blob/master/netstring.c
	 */
	template<class ARS>
	static String ReadStringFromStream(ARS& stream,
		boost::asio::yield_context yc, ssize_t maxMessageLength = -1)
	{
		namespace asio = boost::asio;

		size_t len = 0;
		bool leadingZero = false;

		for (uint_fast8_t readBytes = 0;; ++readBytes) {
			char byte = 0;

			{
				asio::mutable_buffer byteBuf (&byte, 1);
				AsyncRead(stream, byteBuf, yc);
			}

			if (isdigit(byte)) {
				if (readBytes == 9) {
					BOOST_THROW_EXCEPTION(std::invalid_argument("Length specifier must not exceed 9 characters"));
				}

				if (leadingZero) {
					BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (leading zero)"));
				}

				len = len * 10u + size_t(byte - '0');

				if (!readBytes && byte == '0') {
					leadingZero = true;
				}
			} else if (byte == ':') {
				if (!readBytes) {
					BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (no length specifier)"));
				}

				break;
			} else {
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (missing :)"));
			}
		}

		if (maxMessageLength >= 0 && len > maxMessageLength) {
			std::stringstream errorMessage;
			errorMessage << "Max data length exceeded: " << (maxMessageLength / 1024) << " KB";

			BOOST_THROW_EXCEPTION(std::invalid_argument(errorMessage.str()));
		}

		String payload;

		if (len) {
			payload.Append(len, 0);

			asio::mutable_buffer payloadBuf (&*payload.Begin(), payload.GetLength());
			AsyncRead(stream, payloadBuf, yc);
		}

		char trailer = 0;

		{
			asio::mutable_buffer trailerBuf (&trailer, 1);
			AsyncRead(stream, trailerBuf, yc);
		}

		if (trailer != ',') {
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (missing ,)"));
		}

		return std::move(payload);
	}

	static size_t WriteStringToStream(const Stream::Ptr& stream, const String& message);
	static size_t WriteStringToStream(const Shared<AsioTlsStream>::Ptr& stream, const String& message);

	/**
	 * Writes data into a stream using the netstring format and returns bytes written.
	 *
	 * @param stream The stream.
	 * @param str The String that is to be written.
	 *
	 * @return The amount of bytes written.
	 */
	template<class AWS>
	static size_t WriteStringToStream(AWS& stream, const String& str, boost::asio::yield_context yc)
	{
		std::ostringstream msgbuf;
		WriteStringToStream(msgbuf, str);

		String msg = msgbuf.str();
		boost::asio::const_buffer msgBuf (msg.CStr(), msg.GetLength());

		AsyncWrite(stream, msgBuf, yc);

		return msg.GetLength();
	}

	static void WriteStringToStream(std::ostream& stream, const String& message);

private:
	NetString();

	template<class ARS>
	static void AsyncRead(ARS& stream, boost::asio::mutable_buffer mb, boost::asio::yield_context yc)
	{
		while (mb.size()) {
			mb += stream.async_read_some(mb, yc);
		}
	}

	template<class AWS>
	static void AsyncWrite(AWS& stream, boost::asio::const_buffer cb, boost::asio::yield_context yc)
	{
		while (cb.size()) {
			cb += stream.async_write_some(cb, yc);
		}
	}
};

}

#endif /* NETSTRING_H */
