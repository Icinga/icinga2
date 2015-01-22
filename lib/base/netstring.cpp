/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
bool NetString::ReadStringFromStream(const Stream::Ptr& stream, String *str)
{
	/* 16 bytes are enough for the header */
	const size_t header_length = 16;
	size_t read_length;
	char *header = static_cast<char *>(malloc(header_length));

	if (header == NULL)
		BOOST_THROW_EXCEPTION(std::bad_alloc());

	read_length = 0;

	while (read_length < header_length) {
		/* Read one byte. */
		int rc = stream->Read(header + read_length, 1);

		if (rc == 0) {
			if (read_length == 0) {
				free(header);
				return false;
			}

			BOOST_THROW_EXCEPTION(std::runtime_error("Read() failed."));
		}

		ASSERT(rc == 1);

		read_length++;

		if (header[read_length - 1] == ':') {
			break;
		} else if (header_length == read_length) {
			free(header);
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (missing :)"));
		}
	}

	/* no leading zeros allowed */
	if (header[0] == '0' && isdigit(header[1])) {
		free(header);
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (leading zero)"));
	}

	size_t len, i;

	len = 0;
	for (i = 0; i < read_length && isdigit(header[i]); i++) {
		/* length specifier must have at most 9 characters */
		if (i >= 9) {
			free(header);
			BOOST_THROW_EXCEPTION(std::invalid_argument("Length specifier must not exceed 9 characters"));
		}

		len = len * 10 + (header[i] - '0');
	}

	free(header);

	/* read the whole message */
	size_t data_length = len + 1;

	char *data = static_cast<char *>(malloc(data_length));

	if (data == NULL) {
		BOOST_THROW_EXCEPTION(std::bad_alloc());
	}

	size_t rc = stream->Read(data, data_length);
	
	if (rc != data_length)
		BOOST_THROW_EXCEPTION(std::runtime_error("Read() failed."));

	if (data[len] != ',')
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid NetString (missing ,)"));
	
	*str = String(&data[0], &data[len]);

	free(data);

	return true;
}

/**
 * Writes data into a stream using the netstring format.
 *
 * @param stream The stream.
 * @param str The String that is to be written.
 */
void NetString::WriteStringToStream(const Stream::Ptr& stream, const String& str)
{
	std::ostringstream msgbuf;
	msgbuf << str.GetLength() << ":" << str << ",";

	String msg = msgbuf.str();
	stream->Write(msg.CStr(), msg.GetLength());
}
