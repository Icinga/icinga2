// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remote/jsonrpc.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/console.hpp"
#include "base/scriptglobal.hpp"
#include "base/convert.hpp"
#include "base/tlsstream.hpp"
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio/spawn.hpp>

using namespace icinga;

#ifdef I2_DEBUG
/**
 * Determine whether the developer wants to see raw JSON messages.
 *
 * @return Internal.DebugJsonRpc boolean
 */
static bool GetDebugJsonRpcCached()
{
	static int debugJsonRpc = -1;

	if (debugJsonRpc != -1)
		return debugJsonRpc;

	debugJsonRpc = false;

	Namespace::Ptr internal = ScriptGlobal::Get("Internal", &Empty);

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
size_t JsonRpc::SendMessage(const Shared<AsioTlsStream>::Ptr& stream, const Dictionary::Ptr& message)
{
	String json = JsonEncode(message);

#ifdef I2_DEBUG
	if (GetDebugJsonRpcCached())
		std::cerr << ConsoleColorTag(Console_ForegroundBlue) << ">> " << json << ConsoleColorTag(Console_Normal) << "\n";
#endif /* I2_DEBUG */

	return NetString::WriteStringToStream(stream, json);
}

/**
 * Sends a message to the connected peer and returns the bytes sent.
 *
 * @param message The message.
 *
 * @return The amount of bytes sent.
 */
size_t JsonRpc::SendMessage(const Shared<AsioTlsStream>::Ptr& stream, const Dictionary::Ptr& message, boost::asio::yield_context yc)
{
	return JsonRpc::SendRawMessage(stream, JsonEncode(message), yc);
}

 /**
  * Sends a raw message to the connected peer.
  *
  * @param stream ASIO TLS Stream
  * @param json message
  * @param yc Yield context required for ASIO
  *
  * @return bytes sent
  */
size_t JsonRpc::SendRawMessage(const Shared<AsioTlsStream>::Ptr& stream, const String& json, boost::asio::yield_context yc)
{
#ifdef I2_DEBUG
	if (GetDebugJsonRpcCached())
		std::cerr << ConsoleColorTag(Console_ForegroundBlue) << ">> " << json << ConsoleColorTag(Console_Normal) << "\n";
#endif /* I2_DEBUG */

	return NetString::WriteStringToStream(stream, json, yc);
}

/**
 * Reads a message from the connected peer.
 *
 * @param stream ASIO TLS Stream
 * @param maxMessageLength maximum size of bytes read.
 *
 * @return A JSON string
 */

String JsonRpc::ReadMessage(const Shared<AsioTlsStream>::Ptr& stream, ssize_t maxMessageLength)
{
	String jsonString = NetString::ReadStringFromStream(stream, maxMessageLength);

#ifdef I2_DEBUG
	if (GetDebugJsonRpcCached())
		std::cerr << ConsoleColorTag(Console_ForegroundBlue) << "<< " << jsonString << ConsoleColorTag(Console_Normal) << "\n";
#endif /* I2_DEBUG */

	return jsonString;
}

/**
 * Reads a message from the connected peer.
 *
 * @param stream ASIO TLS Stream
 * @param yc Yield Context for ASIO
 * @param maxMessageLength maximum size of bytes read.
 *
 * @return A JSON string
 */
String JsonRpc::ReadMessage(const Shared<AsioTlsStream>::Ptr& stream, boost::asio::yield_context yc, ssize_t maxMessageLength)
{
	String jsonString = NetString::ReadStringFromStream(stream, yc, maxMessageLength);

#ifdef I2_DEBUG
	if (GetDebugJsonRpcCached())
		std::cerr << ConsoleColorTag(Console_ForegroundBlue) << "<< " << jsonString << ConsoleColorTag(Console_Normal) << "\n";
#endif /* I2_DEBUG */

	return jsonString;
}

/**
 * Decode message, enforce a Dictionary
 *
 * @param message JSON string
 *
 * @return Dictionary ptr
 */
Dictionary::Ptr JsonRpc::DecodeMessage(const String& message)
{
	Value value = JsonDecode(message);

	if (!value.IsObjectType<Dictionary>()) {
		BOOST_THROW_EXCEPTION(std::invalid_argument("JSON-RPC"
			" message must be a dictionary."));
	}

	return value;
}
