/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef NETSTRING_H
#define NETSTRING_H

#include "base/i2-base.hpp"
#include "base/stream.hpp"
#include "base/tlsstream.hpp"
#include <memory>
#include <boost/asio/spawn.hpp>

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
	static StreamReadStatus ReadStringFromStream(const Stream::Ptr& stream, String *message, StreamReadContext& context,
		bool may_wait = false, ssize_t maxMessageLength = -1);
	static String ReadStringFromStream(const std::shared_ptr<AsioTlsStream>& stream,
		boost::asio::yield_context yc, ssize_t maxMessageLength = -1);
	static size_t WriteStringToStream(const Stream::Ptr& stream, const String& message);
	static size_t WriteStringToStream(const std::shared_ptr<AsioTlsStream>& stream, const String& message, boost::asio::yield_context yc);
	static void WriteStringToStream(std::ostream& stream, const String& message);

private:
	NetString();
};

}

#endif /* NETSTRING_H */
