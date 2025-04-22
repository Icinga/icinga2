/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef JSON_H
#define JSON_H

#include "base/i2-base.hpp"
#include "base/tlsstream.hpp"
#include <json.hpp>

namespace icinga
{

/**
 * Asynchronous JSON writer interface.
 *
 * This interface is used to write JSON data asynchronously into all kind of streams. It is intended to be used with
 * the @c JsonEncoder class, but it's not limited to. It extends the @c nlohmann::detail::output_adapter_protocol<>
 * interface, thus any class implementing this can be used as an output adapter for the @c nlohmann/json library.
 *
 * @ingroup base
 */
class AsyncJsonWriter : public nlohmann::detail::output_adapter_protocol<char>
{
	virtual void async_put(char ch) = 0;
	virtual void async_write(const char* data, std::size_t size) = 0;
};

/**
 * Asio stream adapter for JSON serialization.
 *
 * This class implements the @c AsyncJsonWriter interface and provides an adapter for writing JSON data to an
 * Asio stream. Similar to the @c nlohmann::detail::output_stream_adapter<> class, this class provides methods
 * for writing characters and strings to an Asio stream asynchronously.
 *
 * In order to satisfy the @c nlohmann::detail::output_adapter_protocol<> interface, this class provides a proper
 * implementation of the @c write_character() and @c write_characters() methods, which just forward the request to
 * the @c async_put() and @c async_write() methods respectively. The type of the underlying Asio stream should be
 * either @c AsioTcpStream or @c AsioTlsStream, or any other type derived from the @c boost::asio::buffered_stream<>
 * class, since every write operation (even with a single char) is directly written to the stream without any extra
 * buffering.
 *
 * @ingroup base
 */
template<typename Stream>
class AsioStreamAdapter : public AsyncJsonWriter
{
public:
	AsioStreamAdapter(Stream& stream, boost::asio::yield_context& yc): m_Stream(stream), m_Yield{yc}
	{
	}

	void write_character(char c) override
	{
		async_put(c);
	}

	void write_characters(const char* s, std::size_t length) override
	{
		async_write(s, length);
	}

	void async_put(char ch) override
	{
		m_Stream.async_write_some(boost::asio::buffer(&ch, 1), m_Yield);
	}

	void async_write(const char* data, std::size_t size) override
	{
		boost::asio::async_write(m_Stream, boost::asio::buffer(data, size), m_Yield);
	}

private:
	Stream& m_Stream;
	boost::asio::yield_context& m_Yield;
};

class String;
class Value;

String JsonEncode(const Value& value, bool pretty_print = false);
Value JsonDecode(const String& data);

}

#endif /* JSON_H */
