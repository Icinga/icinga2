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

#include "i2-jsonrpc.h"

using namespace icinga;

/* based on https://github.com/PeterScott/netstring-c/blob/master/netstring.c */
bool Netstring::ReadStringFromFIFO(FIFO::Ptr fifo, string *str)
{
	size_t buffer_length = fifo->GetSize();
	char *buffer = (char *)fifo->GetReadBuffer();

	/* minimum netstring length is 3 */
	if (buffer_length < 3)
		return false;

	/* no leading zeros allowed */
	if (buffer[0] == '0' && isdigit(buffer[1]))
		throw InvalidArgumentException("Invalid netstring (leading zero)");

	size_t len, i;

	len = 0;
	for (i = 0; i < buffer_length && isdigit(buffer[i]); i++) {
		/* length specifier must have at most 9 characters */
		if (i >= 9)
			throw InvalidArgumentException("Length specifier must not exceed 9 characters");

		len = len * 10 + (buffer[i] - '0');
	}

	/* make sure the buffer is large enough */
	if (i + len + 1 >= buffer_length)
		return false;

	/* check for the colon delimiter */
	if (buffer[i++] != ':')
		throw InvalidArgumentException("Invalid Netstring (missing :)");

	/* check for the comma delimiter after the string */
	if (buffer[i + len] != ',')
		throw InvalidArgumentException("Invalid Netstring (missing ,)");

	*str = string(&buffer[i], &buffer[i + len]);

	/* remove the data from the fifo */
	fifo->Read(NULL, i + len + 1);

	return true;
}

void Netstring::WriteStringToFIFO(FIFO::Ptr fifo, const string& str)
{
	unsigned long len = str.size();
	char strLength[50];
	sprintf(strLength, "%lu:", (unsigned long)len);

	fifo->Write(strLength, strlen(strLength));
	fifo->Write(str.c_str(), len);

	fifo->Write(",", 1);
}
