/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
size_t JsonRpc::SendMessage(const Stream::Ptr& stream, const Dictionary::Ptr& message)
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
size_t JsonRpc::SendMessage(const std::shared_ptr<AsioTlsStream>& stream, const Dictionary::Ptr& message, boost::asio::yield_context yc)
{
	return JsonRpc::SendRawMessage(stream, JsonEncode(message), yc);
}

/**
 * Sends a message to the connected peer and returns the bytes sent.
 *
 * @param message The message.
 *
 * @return The amount of bytes sent.
 */
size_t JsonRpc::SendRawMessage(const std::shared_ptr<AsioTlsStream>& stream, const String& json, boost::asio::yield_context yc)
{
#ifdef I2_DEBUG
	if (GetDebugJsonRpcCached())
		std::cerr << ConsoleColorTag(Console_ForegroundBlue) << ">> " << json << ConsoleColorTag(Console_Normal) << "\n";
#endif /* I2_DEBUG */

	return NetString::WriteStringToStream(stream, json, yc);
}

StreamReadStatus JsonRpc::ReadMessage(const Stream::Ptr& stream, String *message, StreamReadContext& src, bool may_wait, ssize_t maxMessageLength)
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

String JsonRpc::ReadMessage(const std::shared_ptr<AsioTlsStream>& stream, boost::asio::yield_context yc, ssize_t maxMessageLength)
{
	String jsonString = NetString::ReadStringFromStream(stream, yc, maxMessageLength);

#ifdef I2_DEBUG
	if (GetDebugJsonRpcCached())
		std::cerr << ConsoleColorTag(Console_ForegroundBlue) << "<< " << jsonString << ConsoleColorTag(Console_Normal) << "\n";
#endif /* I2_DEBUG */

	return std::move(jsonString);
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
