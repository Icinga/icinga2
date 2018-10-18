/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "base/netstring.hpp"
#include "base/debug.hpp"
#include <sstream>

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

	if (maxMessageLength >= 0 && data_length > maxMessageLength) {
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
 * Writes data into a stream using the netstring format.
 *
 * @param stream The stream.
 * @param str The String that is to be written.
 */
void NetString::WriteStringToStream(std::ostream& stream, const String& str)
{
	stream << str.GetLength() << ":" << str << ",";
}
