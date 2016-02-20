/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef JSONRPC_H
#define JSONRPC_H

#include "base/stream.hpp"
#include "base/dictionary.hpp"
#include "remote/i2-remote.hpp"

namespace icinga
{

/**
 * A JSON-RPC connection.
 *
 * @ingroup remote
 */
class I2_REMOTE_API JsonRpc
{
public:
	static void SendMessage(const Stream::Ptr& stream, const Dictionary::Ptr& message);
	static StreamReadStatus ReadMessage(const Stream::Ptr& stream, String *message, StreamReadContext& src, bool may_wait = false);
	static Dictionary::Ptr DecodeMessage(const String& message);

private:
	JsonRpc(void);
};

}

#endif /* JSONRPC_H */
