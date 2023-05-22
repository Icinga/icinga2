/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/stream.hpp"
#include "base/dictionary.hpp"
#include "base/tlsstream.hpp"
#include "remote/i2-remote.hpp"
#include <memory>
#include <boost/asio/spawn.hpp>

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
	static size_t SendRawMessage(const Shared<AsioTlsStream>::Ptr& stream, const String& json, boost::asio::yield_context yc);

	static String ReadMessage(const Shared<AsioTlsStream>::Ptr& stream, ssize_t maxMessageLength = -1);
	static String ReadMessage(const Shared<AsioTlsStream>::Ptr& stream, boost::asio::yield_context yc, ssize_t maxMessageLength = -1);

	static Dictionary::Ptr DecodeMessage(const String& message);

private:
	JsonRpc();
};

}
