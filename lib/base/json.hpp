/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef JSON_H
#define JSON_H

#include "base/i2-base.hpp"
#include "base/array.hpp"
#include "base/generator.hpp"
#include <boost/asio/spawn.hpp>
#include <json.hpp>

namespace icinga
{

/**
 * AsyncJsonWriter allows writing JSON data to any output stream asynchronously.
 *
 * All users of this class must ensure that the underlying output stream will not perform any asynchronous I/O
 * operations when the @c write_character() or @c write_characters() methods are called. They shall only perform
 * such ops when the @c JsonEncoder allows them to do so by calling the @c MayFlush() method.
 *
 * @ingroup base
 */
class AsyncJsonWriter : public nlohmann::detail::output_adapter_protocol<char>
{
public:
	/**
	 * It instructs the underlying output stream to write any buffered data to wherever it is supposed to go.
	 *
	 * The @c JsonEncoder allows the stream to even perform asynchronous operations in a safe manner by calling
	 * this method with a dedicated @c boost::asio::yield_context object. The stream must not perform any async
	 * I/O operations triggered by methods other than this one. Any attempt to do so will result in undefined behavior.
	 *
	 * However, this doesn't necessarily enforce the stream to really flush its data immediately, but it's up
	 * to the implementation to do whatever it needs to. The encoder just gives it a chance to do so by calling
	 * this method.
	 *
	 * @param yield The yield context to use for asynchronous operations.
	 */
	virtual void MayFlush(boost::asio::yield_context& yield) = 0;
};

/**
 * Adapter class for Boost Beast HTTP messages body to be used with the @c JsonEncoder.
 *
 * This class implements the @c nlohmann::detail::output_adapter_protocol<> interface and provides
 * a way to write JSON data directly into the body of a Boost Beast HTTP message. The adapter is designed
 * to work with Boost Beast HTTP messages that conform to the Beast HTTP message interface and must provide
 * a body type that has a publicly accessible `reader` type that satisfies the Beast BodyReader [^1] requirements.
 *
 * @ingroup base
 *
 * [^1]: https://www.boost.org/doc/libs/1_85_0/libs/beast/doc/html/beast/concepts/BodyReader.html
 */
template<typename BeastHttpMessage>
class BeastHttpMessageAdapter : public AsyncJsonWriter
{
public:
	using BodyType = typename BeastHttpMessage::body_type;
	using Reader = typename BeastHttpMessage::body_type::reader;

	explicit BeastHttpMessageAdapter(BeastHttpMessage& msg) : m_Reader(msg.base(), msg.body()), m_Message{msg}
	{
		boost::system::error_code ec;
		// This never returns an actual error, except when overflowing the max
		// buffer size, which we don't do here.
		m_Reader.init(m_MinPendingBufferSize, ec);
	}

	~BeastHttpMessageAdapter() override
	{
		boost::system::error_code ec;
		// Same here as in the constructor, all the standard Beast HTTP message reader implementations
		// never return an error here, it's just there to satisfy the interface requirements.
		m_Reader.finish(ec);
	}

	void write_character(char c) override
	{
		write_characters(&c, 1);
	}

	void write_characters(const char* s, std::size_t length) override
	{
		boost::system::error_code ec;
		boost::asio::const_buffer buf{s, length};
		while (buf.size()) {
			std::size_t w = m_Reader.put(buf, ec);
			if (ec) {
				BOOST_THROW_EXCEPTION(boost::system::system_error{ec});
			}
			buf += w;
		}
		m_PendingBufferSize += length;
	}

	void Flush(boost::asio::yield_context& yield) override
	{
		if (m_PendingBufferSize >= m_MinPendingBufferSize) {
			m_Message.Write(yield);
			m_PendingBufferSize = 0;
		}
	}

private:
	Reader m_Reader;
	BeastHttpMessage& m_Message;
	std::size_t m_PendingBufferSize{0}; // Tracks the size of the pending buffer to avoid unnecessary writes.
	// Minimum size of the pending buffer before we flush the data to the underlying stream.
	static constexpr std::size_t m_MinPendingBufferSize = 4096;
};

class String;
class Value;

/**
 * JSON encoder.
 *
 * This class can be used to encode Icinga Value types into JSON format and write them to an output stream.
 * The supported stream types include any @c std::ostream like objects and our own @c AsyncJsonWriter, which
 * allows writing JSON data to an Asio stream asynchronously. The nlohmann/json library already provides
 * full support for the former stream type, while the latter is fully implemented by our own and satisfies the
 * @c nlohmann::detail::output_adapter_protocol<> interface as well.
 *
 * The JSON encoder generates most of the low level JSON tokens, but it still relies on the already existing
 * @c nlohmann::detail::serializer<> class to dump numbers and ASCII validated JSON strings. This means that the
 * encoder doesn't perform any kind of JSON validation or escaping on its own, but simply delegates all this kind
 * of work to serializer<>.
 *
 * The generated JSON can be either prettified or compact, depending on your needs. The prettified JSON object
 * is indented with 4 spaces and grows linearly with the depth of the object tree.
 *
 * @ingroup base
 */
class JsonEncoder
{
public:
	explicit JsonEncoder(std::string& output, bool prettify = false);
	explicit JsonEncoder(std::basic_ostream<char>& stream, bool prettify = false);
	explicit JsonEncoder(nlohmann::detail::output_adapter_t<char> w, bool prettify = false);

	void Encode(const Value& value, boost::asio::yield_context* yc = nullptr);

private:
	void EncodeArray(const Array::Ptr& array, boost::asio::yield_context* yc);
	void EncodeValueGenerator(const ValueGenerator::Ptr& generator, boost::asio::yield_context* yc);

	template<typename Iterable, typename ValExtractor>
	void EncodeObject(const Iterable& container, const ValExtractor& extractor, boost::asio::yield_context* yc);

	void EncodeNlohmannJson(const nlohmann::json& json) const;
	void EncodeNumber(double value) const;

	void Write(const std::string_view& sv) const;
	void BeginContainer(char openChar);
	void EndContainer(char closeChar, bool isContainerEmpty = false);
	void WriteSeparatorAndIndentStrIfNeeded(bool emitComma) const;

	// The number of spaces to use for indentation in prettified JSON.
	static constexpr uint8_t m_IndentSize = 4;

	bool m_Pretty; // Whether to pretty-print the JSON output.
	unsigned m_Indent{0}; // The current indentation level for pretty-printing.
	/**
	 * Pre-allocate for 8 levels of indentation for pretty-printing.
	 *
	 * This is used to avoid reallocating the string on every indent level change.
	 * The size of this string is dynamically adjusted if the indentation level exceeds its initial size at some point.
	 */
	std::string m_IndentStr{8*m_IndentSize, ' '};

	// The output stream adapter for writing JSON data. This can be either a std::ostream or an Asio stream adapter.
	nlohmann::detail::output_adapter_t<char> m_Writer;

	/**
	 * This class wraps any @c nlohmann::detail::output_adapter_t<char> writer and provides a method to flush it as
	 * required. Only @c AsyncJsonWriter supports the flush operation, however, this class is also safe to use with
	 * other writer types and the flush method does nothing for them.
	 */
	class Flusher {
	public:
		explicit Flusher(const nlohmann::detail::output_adapter_t<char>& w);
		void FlushIfSafe(boost::asio::yield_context* yc) const;

	private:
		AsyncJsonWriter* m_AsyncWriter;
	} m_Flusher;
};

String JsonEncode(const Value& value, bool prettify = false);
void JsonEncode(const Value& value, std::ostream& os, bool prettify = false);
Value JsonDecode(const String& data);

}

#endif /* JSON_H */
