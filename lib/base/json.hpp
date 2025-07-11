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

	void FlushIfSafe(boost::asio::yield_context* yc) const;

	void Write(const std::string_view& sv) const;
	void BeginContainer(char openChar);
	void EndContainer(char closeChar, bool isContainerEmpty = false);
	void WriteSeparatorAndIndentStrIfNeeded(bool emitComma) const;

	// The number of spaces to use for indentation in prettified JSON.
	static constexpr uint8_t m_IndentSize = 4;

	const bool m_IsAsyncWriter; // Whether the writer is an instance of AsyncJsonWriter.
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
};

String JsonEncode(const Value& value, bool prettify = false);
void JsonEncode(const Value& value, std::ostream& os, bool prettify = false);
Value JsonDecode(const String& data);

}

#endif /* JSON_H */
