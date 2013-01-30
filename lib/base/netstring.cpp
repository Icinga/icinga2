/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-base.h"

using namespace icinga;

/**
 * Reads data from a stream in netString format.
 *
 * @param stream The stream to read from.
 * @param[out] str The String that has been read from the IOQueue.
 * @returns true if a complete String was read from the IOQueue, false otherwise.
 * @exception invalid_argument The input stream is invalid.
 * @see https://github.com/PeterScott/netString-c/blob/master/netString.c
 */
bool NetString::ReadStringFromStream(const Stream::Ptr& stream, String *str)
{
	/* 16 bytes are enough for the header */
	size_t peek_length, buffer_length = 16;
	char *buffer = static_cast<char *>(malloc(buffer_length));

	if (buffer == NULL)
		throw_exception(bad_alloc());

	peek_length = stream->Peek(buffer, buffer_length);

	/* minimum netString length is 3 */
	if (peek_length < 3) {
		free(buffer);
		return false;
	}

	/* no leading zeros allowed */
	if (buffer[0] == '0' && isdigit(buffer[1])) {
		free(buffer);
		throw_exception(invalid_argument("Invalid netString (leading zero)"));
	}

	size_t len, i;

	len = 0;
	for (i = 0; i < peek_length && isdigit(buffer[i]); i++) {
		/* length specifier must have at most 9 characters */
		if (i >= 9) {
			free(buffer);
			throw_exception(invalid_argument("Length specifier must not exceed 9 characters"));
		}

		len = len * 10 + (buffer[i] - '0');
	}

	/* read the whole message */
	buffer_length = i + 1 + len + 1;

	char *new_buffer = static_cast<char *>(realloc(buffer, buffer_length));

	if (new_buffer == NULL) {
		free(buffer);
		throw_exception(bad_alloc());
	}

	buffer = new_buffer;

	peek_length = stream->Peek(buffer, buffer_length);

	if (peek_length < buffer_length)
		return false;

	/* check for the colon delimiter */
	if (buffer[i] != ':') {
		free(buffer);
		throw_exception(invalid_argument("Invalid NetString (missing :)"));
	}

	/* check for the comma delimiter after the String */
	if (buffer[i + 1 + len] != ',') {
		free(buffer);
		throw_exception(invalid_argument("Invalid NetString (missing ,)"));
	}

	*str = String(&buffer[i + 1], &buffer[i + 1 + len]);

	free(buffer);

	/* remove the data from the stream */
	stream->Read(NULL, peek_length);

	return true;
}

/**
 * Writes data into a stream using the netString format.
 *
 * @param stream The stream.
 * @param str The String that is to be written.
 */
void NetString::WriteStringToStream(const Stream::Ptr& stream, const String& str)
{
	stringstream prefixbuf;
	prefixbuf << str.GetLength() << ":";

	String prefix = prefixbuf.str();
	stream->Write(prefix.CStr(), prefix.GetLength());
	stream->Write(str.CStr(), str.GetLength());
	stream->Write(",", 1);
}
