/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef JSON_H
#define JSON_H

#include "base/i2-base.hpp"
#include "base/dictionary.hpp"
#include "base/namespace.hpp"
#include "base/objectlock.hpp"
#include "base/tlsstream.hpp"
#include "base/utility.hpp"
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

/**
 * JSON encoder.
 *
 * This class can be used to encode Icinga Value types into JSON format and write them to an output stream.
 * The supported stream types include std::ostream and AsioStreamAdapter. The nlohmann/json library already
 * provides a full support for the former stream type, while the latter is fully implemented by our own and
 * satisfies the @c nlohmann::detail::output_adapter_protocol<> interface. It is used for writing the produced
 * JSON directly to an Asio either TCP or TLS stream and doesn't require any additional buffering other than
 * the one used by the Asio buffered_stream<> class internally.
 *
 * The JSON encoder generates most of the low level JSON tokens, but it still relies on the already existing
 * @c nlohmann::detail::serializer<> class to dump numbers and ascii validated JSON strings. This means that the
 * encoder doesn't perform any kind of JSON validation or escaping on its own, but simply delegates all this kind
 * of work to serializer<>. However, Strings are UTF-8 validated beforehand using the @c Utility::ValidateUTF8()
 * function and only the validated (copy of the original) String is passed to the serializer.
 *
 * The generated JSON can be either prettified or compact, depending on your needs. The prettified JSON object
 * is indented with 4 spaces and grows linearly with the depth of the object tree.
 *
 * @note The JSON serialization logic of this encoder is heavily inspired by the @c nlohmann::detail::serializer<>.
 *
 * @ingroup base
 */
class JsonEncoder
{
#define RELEASE_OLOCK_IF_ASYNC_WRITER(writer, olock) \
	if (std::is_base_of<AsyncJsonWriter, decltype(writer)>::value) { \
		olock.Unlock(); \
	}

public:
	explicit JsonEncoder(std::basic_ostream<char>& stream, bool prettify = false)
		: JsonEncoder{nlohmann::detail::output_adapter<char>(stream), prettify}
	{
	}

	explicit JsonEncoder(const std::shared_ptr<AsioStreamAdapter<AsioTlsStream>>& writer, bool prettify = false)
		: JsonEncoder{nlohmann::detail::output_adapter_t<char>(writer), prettify}
	{
	}

	void Encode(const Value& value, unsigned currentIndent = 0);

private:
	JsonEncoder(nlohmann::detail::output_adapter_t<char> stream, bool pretty)
		: m_Pretty(pretty), m_IndentStr(32, ' '), m_Writer(std::move(stream))
	{}

	/**
	 * Serializes a value into JSON using the serializer and writes it directly to @c m_Writer.
	 *
	 * @tparam T Type of the value to encode via the serializer.
	 * @param value The value to validate and encode using the serializer.
	 */
	template<typename T>
	void EncodeValidatedJson(T&& value) const
	{
		nlohmann::detail::serializer<nlohmann::json> s(m_Writer, ' ', nlohmann::json::error_handler_t::strict);
		s.dump(nlohmann::json(std::forward<T>(value)), m_Pretty, true, 0, 0);
	}

	/**
	 * Encodes an Icinga 2 object (Namespace or Dictionary) into JSON and writes it to @c m_Writer.
	 *
	 * @tparam Iterable Type of the container (Namespace or Dictionary).
	 * @tparam ValExtractor Type of the value extractor function used to extract values from the container's iterator.
	 *
	 * @param container The container to JSON serialize.
	 * @param currentIndent The current indentation level.
	 * @param extractor The value extractor function used to extract values from the container's iterator.
	 */
	template<typename Iterable, typename ValExtractor>
	void EncodeObject(const Iterable& container, unsigned currentIndent, const ValExtractor& extractor)
	{
		static_assert(std::is_same_v<Iterable, Namespace::Ptr> || std::is_same_v<Iterable, Dictionary::Ptr>,
			"Container must be a Namespace or Dictionary");

		ObjectLock olock(container);
		auto begin(container->Begin());
		auto end(container->End());
		// Release the object lock if we're writing to an Asio stream, i.e. every write operation
		// is asynchronous which may cause the coroutine to yield and later resume on another thread.
		RELEASE_OLOCK_IF_ASYNC_WRITER(*m_Writer, olock);

		if (m_Pretty) {
			m_Writer->write_characters("{\n", 2);

			auto newIndent = currentIndent + m_IndentSize;
			if (m_IndentStr.size() < newIndent) {
				m_IndentStr.resize(m_IndentStr.size() * 2, ' ');
			}

			for (auto it(begin); it != end; ++it) {
				// Add a comma before the next key-value pair, but only if it's not the first one.
				if (it != begin) {
					m_Writer->write_characters(",\n", 2);
				}
				m_Writer->write_characters(m_IndentStr.c_str(), newIndent);
				EncodeValidatedJson(Utility::ValidateUTF8(it->first));
				m_Writer->write_characters(": ", 2);
				Encode(extractor(it->second), newIndent);
			}

			m_Writer->write_character('\n');
			m_Writer->write_characters(m_IndentStr.c_str(), currentIndent);
			m_Writer->write_character('}');
		} else {
			m_Writer->write_character('{');
			for (auto it(begin); it != end; ++it) {
				// Add a comma before the next key-value pair, but not before the first one.
				if (it != begin) {
					m_Writer->write_character(',');
				}
				EncodeValidatedJson(Utility::ValidateUTF8(it->first));
				m_Writer->write_character(':');
				Encode(extractor(it->second), currentIndent);
			}
			m_Writer->write_character('}');
		}
	}

private:
	// The number of spaces to use for indentation in prettified JSON.
	static constexpr uint8_t m_IndentSize = 4;

	bool m_Pretty; // Whether to pretty-print the JSON output.
	// The pre-allocated indent characters for pretty-printing.
	// By default, 32 ' ' characters (bytes) are allocated which can be used to indent JSON object trees up to
	// 8 levels deep (4 spaces per level) without reallocation. Otherwise, when encountering a deeper JSON tree,
	// this will be resized to the required size on the fly and doubled each time.
	std::string m_IndentStr;

	// The output stream adapter for writing JSON data.
	// This can be either a std::ostream or an Asio stream adapter.
	nlohmann::detail::output_adapter_t<char> m_Writer;
};

String JsonEncode(const Value& value, bool pretty_print = false);
void JsonEncode(const Value& value, std::ostream& os, bool prettify = false);
Value JsonDecode(const String& data);

}

#endif /* JSON_H */
