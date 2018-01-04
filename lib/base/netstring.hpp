/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/i2-base.hpp"
#include "base/stream.hpp"

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
	static StreamReadStatus ReadStringFromStream(const Stream::Ptr& stream, String *message, StreamReadContext& context, bool may_wait = false);
	static size_t WriteStringToStream(const Stream::Ptr& stream, const String& message);
	static void WriteStringToStream(std::ostream& stream, const String& message);

private:
	NetString();
};

}

#endif /* NETSTRING_H */
