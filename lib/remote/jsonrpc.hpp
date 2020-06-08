/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef JSONRPC_H
#define JSONRPC_H

#include "base/console.hpp"
#include "base/netstring.hpp"
#include "base/stream.hpp"
#include "base/dictionary.hpp"
#include "base/tlsstream.hpp"
#include "remote/i2-remote.hpp"
#include <iostream>
#include <memory>
#include <boost/asio/spawn.hpp>
#include <utility>

namespace icinga
{

/**
 * A JSON-RPC connection.
 *
 * @ingroup remote
 */
class JsonRpc
{
public:
	static size_t SendMessage(const Shared<AsioTlsStream>::Ptr& stream, const Dictionary::Ptr& message);
	static size_t SendMessage(const Shared<AsioTlsStream>::Ptr& stream, const Dictionary::Ptr& message, boost::asio::yield_context yc);

	 /**
	  * Sends a raw message to the connected peer.
	  *
	  * @param stream ASIO TLS Stream
	  * @param json message
	  * @param yc Yield context required for ASIO
	  *
	  * @return bytes sent
	  */
	template<class AWS>
	static size_t SendRawMessage(AWS& stream, const String& json, boost::asio::yield_context yc)
	{
#ifdef I2_DEBUG
		if (GetDebugJsonRpcCached())
			std::cerr << ConsoleColorTag(Console_ForegroundBlue) << ">> " << json << ConsoleColorTag(Console_Normal) << "\n";
#endif /* I2_DEBUG */

		return NetString::WriteStringToStream(stream, json, yc);
	}

	static String ReadMessage(const Shared<AsioTlsStream>::Ptr& stream, ssize_t maxMessageLength = -1);

	/**
	 * Reads a message from the connected peer.
	 *
	 * @param stream ASIO TLS Stream
	 * @param yc Yield Context for ASIO
	 * @param maxMessageLength maximum size of bytes read.
	 *
	 * @return A JSON string
	 */
	template<class ARS>
	static String ReadMessage(ARS& stream, boost::asio::yield_context yc, ssize_t maxMessageLength = -1)
	{
		String jsonString = NetString::ReadStringFromStream(stream, yc, maxMessageLength);

#ifdef I2_DEBUG
		if (GetDebugJsonRpcCached())
			std::cerr << ConsoleColorTag(Console_ForegroundBlue) << "<< " << jsonString << ConsoleColorTag(Console_Normal) << "\n";
#endif /* I2_DEBUG */

		return std::move(jsonString);
	}

	static Dictionary::Ptr DecodeMessage(const String& message);

private:
	JsonRpc();

#ifdef I2_DEBUG
	static bool GetDebugJsonRpcCached();
#endif /* I2_DEBUG */
};

}

#endif /* JSONRPC_H */
