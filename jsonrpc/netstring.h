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

#ifndef NETSTRING_H
#define NETSTRING_H

namespace icinga
{

/**
 * Thrown when an invalid netstring was encountered while reading from a FIFO.
 *
 * @ingroup jsonrpc
 */
DEFINE_EXCEPTION_CLASS(InvalidNetstringException);

/**
 * Helper functions for reading/writing messages in the netstring format.
 *
 * @see http://cr.yp.to/proto/netstrings.txt
 *
 * @ingroup jsonrpc
 */
class I2_JSONRPC_API Netstring
{
public:
	static bool ReadStringFromFIFO(const FIFO::Ptr& fifo, string *message);
	static void WriteStringToFIFO(const FIFO::Ptr& fifo, const string& message);

private:
	Netstring(void);
};

}

#endif /* NETSTRING_H */
