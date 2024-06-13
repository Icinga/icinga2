/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/netstring.hpp"
#include "base/debug.hpp"
#include "base/tlsstream.hpp"
#include <cstdint>
#include <memory>
#include <sstream>
#include <utility>
#include <boost/asio/buffer.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/write.hpp>

using namespace icinga;

/**
 * Reads data from a stream in netstring format.
 *
 * @param stream The stream to read from.
 * @param[out] str The String that has been read from the IOQueue.
 * @returns true if a complete String was read from the IOQueue, false otherwise.
 * @exception invalid_argument The input stream is invalid.
 * @see https://github.com/PeterScott/netstring-c/blob/master/netstring.c
 */
StreamReadStatus NetString::ReadStringFromStream(const Stream::Ptr& stream, String *str, StreamReadContext& context,
	bool may_wait, ssize_t maxMessageLength)
{
	if (context.Eof)
		return StatusEof;

	if (context.MustRead) {
		if (!context.FillFromStream(stream, may_wait)) {
			context.Eof = true;
			return StatusEof;
		}

		context.MustRead = false;
	}

	size_t header_length = 0;

	for (size_t i = 0; i < context.Size; i++) {
		if (context.Buffer[i] == ':') {
			header_length = i;

			/* make sure there's a header */
			if (header_length == 0)
				BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (no length specifier)"));

			break;
		} else if (i > 16)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (missing :)"));
	}

	if (header_length == 0) {
		context.MustRead = true;
		return StatusNeedData;
	}

	/* no leading zeros allowed */
	if (context.Buffer[0] == '0' && isdigit(context.Buffer[1]))
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (leading zero)"));

	size_t len, i;

	len = 0;
	for (i = 0; i < header_length && isdigit(context.Buffer[i]); i++) {
		/* length specifier must have at most 9 characters */
		if (i >= 9)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Length specifier must not exceed 9 characters"));

		len = len * 10 + (context.Buffer[i] - '0');
	}

	/* read the whole message */
	size_t data_length = len + 1;

	if (maxMessageLength >= 0 && data_length > (size_t)maxMessageLength) {
		std::stringstream errorMessage;
		errorMessage << "Max data length exceeded: " << (maxMessageLength / 1024) << " KB";

		BOOST_THROW_EXCEPTION(std::invalid_argument(errorMessage.str()));
	}

	char *data = context.Buffer + header_length + 1;

	if (context.Size < header_length + 1 + data_length) {
		context.MustRead = true;
		return StatusNeedData;
	}

	if (data[len] != ',')
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (missing ,)"));

	*str = String(&data[0], &data[len]);

	context.DropData(header_length + 1 + len + 1);

	return StatusNewItem;
}

/**
 * Writes data into a stream using the netstring format and returns bytes written.
 *
 * @param stream The stream.
 * @param str The String that is to be written.
 *
 * @return The amount of bytes written.
 */
size_t NetString::WriteStringToStream(const Stream::Ptr& stream, const String& str)
{
	std::ostringstream msgbuf;
	WriteStringToStream(msgbuf, str);

	String msg = msgbuf.str();
	stream->Write(msg.CStr(), msg.GetLength());
	return msg.GetLength();
}

/**
 * Reads data from a stream in netstring format.
 *
 * @param stream The stream to read from.
 * @returns The String that has been read from the IOQueue.
 * @exception invalid_argument The input stream is invalid.
 * @see https://github.com/PeterScott/netstring-c/blob/master/netstring.c
 */
String NetString::ReadStringFromStream(const AsioTlsStream::Ptr& stream,
	ssize_t maxMessageLength)
{
	namespace asio = boost::asio;

	size_t len = 0;
	bool leadingZero = false;

	for (uint_fast8_t readBytes = 0;; ++readBytes) {
		char byte = 0;

		{
			asio::mutable_buffer byteBuf (&byte, 1);
			asio::read(*stream, byteBuf);
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
		asio::read(*stream, payloadBuf);
	}

	char trailer = 0;

	{
		asio::mutable_buffer trailerBuf (&trailer, 1);
		asio::read(*stream, trailerBuf);
	}

	if (trailer != ',') {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (missing ,)"));
	}

	return payload;
}

/**
 * Reads data from a stream in netstring format.
 *
 * @param stream The stream to read from.
 * @returns The String that has been read from the IOQueue.
 * @exception invalid_argument The input stream is invalid.
 * @see https://github.com/PeterScott/netstring-c/blob/master/netstring.c
 */
String NetString::ReadStringFromStream(const AsioTlsStream::Ptr& stream,
	boost::asio::yield_context yc, ssize_t maxMessageLength)
{
	namespace asio = boost::asio;

	size_t len = 0;
	bool leadingZero = false;

	for (uint_fast8_t readBytes = 0;; ++readBytes) {
		char byte = 0;

		{
			asio::mutable_buffer byteBuf (&byte, 1);
			asio::async_read(*stream, byteBuf, yc);
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
		asio::async_read(*stream, payloadBuf, yc);
	}

	char trailer = 0;

	{
		asio::mutable_buffer trailerBuf (&trailer, 1);
		asio::async_read(*stream, trailerBuf, yc);
	}

	if (trailer != ',') {
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (missing ,)"));
	}

	return payload;
}

/**
 * Writes data into a stream using the netstring format and returns bytes written.
 *
 * @param stream The stream.
 * @param str The String that is to be written.
 *
 * @return The amount of bytes written.
 */
size_t NetString::WriteStringToStream(const AsioTlsStream::Ptr& stream, const String& str)
{
	namespace asio = boost::asio;

	std::ostringstream msgbuf;
	WriteStringToStream(msgbuf, str);

	String msg = msgbuf.str();
	asio::const_buffer msgBuf (msg.CStr(), msg.GetLength());

	asio::write(*stream, msgBuf);

	return msg.GetLength();
}

/**
 * Writes data into a stream using the netstring format and returns bytes written.
 *
 * @param stream The stream.
 * @param str The String that is to be written.
 *
 * @return The amount of bytes written.
 */
size_t NetString::WriteStringToStream(const AsioTlsStream::Ptr& stream, const String& str, boost::asio::yield_context yc)
{
	namespace asio = boost::asio;

	std::ostringstream msgbuf;
	WriteStringToStream(msgbuf, str);

	String msg = msgbuf.str();
	asio::const_buffer msgBuf (msg.CStr(), msg.GetLength());

	asio::async_write(*stream, msgBuf, yc);

	return msg.GetLength();
}

/**
 * Writes data into a stream using the netstring format.
 *
 * @param stream The stream.
 * @param str The String that is to be written.
 */
void NetString::WriteStringToStream(std::ostream& stream, const String& str)
{
	stream << str.GetLength() << ":" << str << ",";
}
