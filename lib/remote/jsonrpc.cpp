/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "remote/jsonrpc.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"

using namespace icinga;

/**
 * Sends a message to the connected peer.
 *
 * @param message The message.
 */
void JsonRpc::SendMessage(const Stream::Ptr& stream, const Dictionary::Ptr& message)
{
	String json = JsonEncode(message);
	NetString::WriteStringToStream(stream, json);
}

StreamReadStatus JsonRpc::ReadMessage(const Stream::Ptr& stream, String *message, StreamReadContext& src, bool may_wait)
{
	String jsonString;
	StreamReadStatus srs = NetString::ReadStringFromStream(stream, &jsonString, src, may_wait);

	if (srs != StatusNewItem)
		return srs;

	*message = jsonString;

	return StatusNewItem;
}

Dictionary::Ptr JsonRpc::DecodeMessage(const String& message)
{
	Value value = JsonDecode(message);

	if (!value.IsObjectType<Dictionary>()) {
		BOOST_THROW_EXCEPTION(std::invalid_argument("JSON-RPC"
		    " message must be a dictionary."));
	}

	return value;
}
