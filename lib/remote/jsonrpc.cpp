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

#include "remote/jsonrpc.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/console.hpp"
#include "base/scriptglobal.hpp"
#include "base/convert.hpp"
#include <iostream>

using namespace icinga;

#ifdef I2_DEBUG
static bool GetDebugJsonRpcCached()
{
	static int debugJsonRpc = -1;

	if (debugJsonRpc != -1)
		return debugJsonRpc;

	debugJsonRpc = false;

	Dictionary::Ptr internal = ScriptGlobal::Get("Internal", &Empty);

	if (!internal)
		return false;

	Value vdebug;

	if (!internal->Get("DebugJsonRpc", &vdebug))
		return false;

	debugJsonRpc = Convert::ToLong(vdebug);

	return debugJsonRpc;
}
#endif /* I2_DEBUG */

/**
 * Sends a message to the connected peer and returns the bytes sent.
 *
 * @param message The message.
 *
 * @return The amount of bytes sent.
 */
size_t JsonRpc::SendMessage(const Stream::Ptr& stream, const Dictionary::Ptr& message)
{
	String json = JsonEncode(message);

#ifdef I2_DEBUG
	if (GetDebugJsonRpcCached())
		std::cerr << ConsoleColorTag(Console_ForegroundBlue) << ">> " << json << ConsoleColorTag(Console_Normal) << "\n";
#endif /* I2_DEBUG */

	return NetString::WriteStringToStream(stream, json);
}

StreamReadStatus JsonRpc::ReadMessage(const Stream::Ptr& stream, String *message, StreamReadContext& src, bool may_wait, size_t maxMessageLength)
{
	String jsonString;
	StreamReadStatus srs = NetString::ReadStringFromStream(stream, &jsonString, src, may_wait, maxMessageLength);

	if (srs != StatusNewItem)
		return srs;

	*message = jsonString;

#ifdef I2_DEBUG
	if (GetDebugJsonRpcCached())
		std::cerr << ConsoleColorTag(Console_ForegroundBlue) << "<< " << jsonString << ConsoleColorTag(Console_Normal) << "\n";
#endif /* I2_DEBUG */

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
