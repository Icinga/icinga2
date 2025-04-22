/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef JSON_H
#define JSON_H

#include "base/i2-base.hpp"
#include "base/dictionary.hpp"
#include "base/namespace.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include <boost/asio/spawn.hpp>
#include <json.hpp>
#include <string>

namespace icinga
{

/**
 * AsyncJsonWriter allows writing JSON data to any output stream asynchronously.
 *
 * All users of this class must ensure that the underlying output stream will not perform any asynchronous I/O
 * operations when the @c write_character() or @c write_characters() methods are called. They shall only perform
 * such ops when the @c JsonEncoder allows them to do so by calling the @c Flush() method.
 *
 * @ingroup base
 */
class AsyncJsonWriter : public nlohmann::detail::output_adapter_protocol<char>
{
public:
	/**
	 * Flush instructs the underlying output stream to write any buffered data to wherever it is supposed to go.
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
	virtual void Flush(boost::asio::yield_context& yield) = 0;
};

/**
 * Adapter class for writing JSON data to a std::string.
 *
 * This class implements the @c nlohmann::detail::output_adapter_protocol<> interface and provides
 * a way to write JSON data directly into a std::string and overcomes the overhead of using a
 * @c std::ostringstream for JSON serialization.
 *
 * The constructor takes a reference to a std::string, which is used as the output buffer for the JSON data,
 * and you must ensure that reference remains valid for the lifetime of the StringOutputAdapter instance.
 *
 * @ingroup base
 */
class StringOutputAdapter final : public nlohmann::detail::output_adapter_protocol<char>
{
public:
	explicit StringOutputAdapter(std::string& str) : m_OutStr(str)
	{}

	void write_character(char c) override
	{
		m_OutStr.append(1, c);
	}

	void write_characters(const char* s, std::size_t length) override
	{
		m_OutStr.append(s, length);
	}

private:
	std::string& m_OutStr;
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
 * @ingroup base
 */
class JsonEncoder
{
public:
	explicit JsonEncoder(std::string& output, bool prettify = false);
	explicit JsonEncoder(std::basic_ostream<char>& stream, bool prettify = false);
	explicit JsonEncoder(nlohmann::detail::output_adapter_t<char> stream, bool prettify = false);

	/**
	 * Encodes a single value into JSON and writes it to the underlying output stream.
	 *
	 * This method is the main entry point for encoding JSON data. It takes a value of any type that can be
	 * represented by our @c Value class and encodes it into JSON in an efficient manner. If prettifying is
	 * enabled, the JSON output will be formatted with indentation and newlines for better readability, and
	 * the final JSON will also be terminated by a newline character.
	 *
	 * It can be used to incrementally build a JSON object or array by adding elements one by one.
	 *
	 * @tparam T The type of the value to encode.
	 *
	 * @param value The value to encode into JSON format.
	 */
	template<typename T>
	void Encode(T&& value)
	{
		EncodeImpl(std::forward<T>(value));
		// If we are at the top level of the JSON object and prettifying is enabled, we need
		// to end the JSON with a newline character to ensure that the output is properly formatted.
		if (m_Indent == 0 && m_Pretty) {
			Write("\n");
		}
	}

private:
	void EncodeImpl(const Value& value);

	void Write(const std::string_view& sv) const;
	void BeginContainer(char openChar);
	void EndContainer(char closeChar, bool isContainerEmpty = false);
	void WriteSeparatorAndIndentStrIfNeeded(int count) const;

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
	 * @param extractor The value extractor function used to extract values from the container's iterator.
	 */
	template<typename Iterable, typename ValExtractor>
	void EncodeObject(const Iterable& container, const ValExtractor& extractor)
	{
		static_assert(std::is_same_v<Iterable, Namespace::Ptr> || std::is_same_v<Iterable, Dictionary::Ptr>,
			"Container must be a Namespace or Dictionary");

		BeginContainer('{');
		ObjectLock olock(container);
		int count = 0;
		for (const auto& [key, val] : container) {
			WriteSeparatorAndIndentStrIfNeeded(count++);
			EncodeValidatedJson(Utility::ValidateUTF8(key));
			Write(m_Pretty ? ": " : ":");

			EncodeImpl(extractor(val));
		}
		EndContainer('}', container->GetLength() == 0);
	}

private:
	// The number of spaces to use for indentation in prettified JSON.
	static constexpr uint8_t m_IndentSize = 4;

	bool m_Pretty; // Whether to pretty-print the JSON output.
	unsigned m_Indent; // The current indentation level for pretty-printing.
	/**
	 * The pre-allocated indent characters for pretty-printing.
	 *
	 * By default, 32 @c ' ' characters (bytes) are allocated which can be used to indent JSON object trees up to
	 * 8 levels deep (4 spaces per level) without reallocation. Otherwise, when encountering a deeper JSON tree,
	 * this will be resized to the required size on the fly and doubled each time.
	 */
	std::string m_IndentStr;

	// The output stream adapter for writing JSON data. This can be either a std::ostream or an Asio stream adapter.
	nlohmann::detail::output_adapter_t<char> m_Writer;
};

String JsonEncode(const Value& value, bool pretty_print = false);
void JsonEncode(const Value& value, std::ostream& os, bool prettify = false);
Value JsonDecode(const String& data);

}

#endif /* JSON_H */
