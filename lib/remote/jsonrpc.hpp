/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef JSONRPC_H
#define JSONRPC_H

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
	static size_t SendMessage(const Stream::Ptr& stream, const Dictionary::Ptr& message);
	static size_t SendMessage(const std::shared_ptr<AsioTlsStream>& stream, const Dictionary::Ptr& message, boost::asio::yield_context yc);
	static StreamReadStatus ReadMessage(const Stream::Ptr& stream, String *message, StreamReadContext& src, bool may_wait = false, ssize_t maxMessageLength = -1);
	static String ReadMessage(const std::shared_ptr<AsioTlsStream>& stream, boost::asio::yield_context yc, ssize_t maxMessageLength = -1);
	static Dictionary::Ptr DecodeMessage(const String& message);

private:
	JsonRpc();
};

}

#endif /* JSONRPC_H */
