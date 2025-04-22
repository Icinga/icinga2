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
#include <stack>

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
		m_Stream.async_write_some(boost::asio::const_buffer(&ch, 1), m_Yield);
	}

	void async_write(const char* data, std::size_t size) override
	{
		boost::asio::async_write(m_Stream, boost::asio::const_buffer(data, size), m_Yield);
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
 * @ingroup base
 */
class JsonEncoder
{
public:
	explicit JsonEncoder(std::basic_ostream<char>& stream, bool prettify = false)
		: JsonEncoder{nlohmann::detail::output_adapter<char>(stream), prettify}
	{
	}

	explicit JsonEncoder(const std::shared_ptr<AsioStreamAdapter<AsioTlsStream>>& writer, bool prettify = false)
		: JsonEncoder{nlohmann::detail::output_adapter_t<char>(writer), prettify}
	{
	}

	/**
	 * Ctrls is an enum class that defines control flags for the JSON encoder.
	 *
	 * These flags are used to signal the beginning and end of JSON objects and arrays, as well as to handle
	 * miscellaneous tasks like emitting commas and newlines. Each control flag corresponds to a specific action
	 * that the encoder should take when encountering it. These commands can be used to incrementally encode JSON
	 * data without having to create a full JSON object or array at once but rather adding elements to it as they
	 * become available. Once the encoding is complete, the @c Flush command must be used to flush all pending unclosed
	 * JSON objects and arrays to the output stream.
	 */
	enum Ctrls {
		BeginObject, // Emit the beginning of a JSON object.
		EndObject,   // Emit the end of a JSON object.
		EmptyObject, // Emit an empty JSON object "{}".
		BeginArray, // Emit the beginning of a JSON array.
		EndArray, // Emit the end of a JSON array.
		EmptyArray, // Emit an empty JSON array "[]".
		Comma, // Emit a comma to separate JSON elements. Might be followed by a newline character if prettifying is enabled.
		NewLine, // Emit a newline character. This is used to prettify the JSON output.
		Flush, // Flush all the pending (unclosed) JSON objects and arrays to the output stream.
	};

	/**
	 * Field is a helper type to represent a key-value pair of JSON objects.
	 *
	 * This is used as an attempt to provide if possible a zero-copy way of passing key-value pairs
	 * to the @c JsonEncoder::Encode() method as opposed to e.g. using a std::pair<>. This can be used
	 * to incrementally add key-value pairs to a previously started but not yet closed JSON object.
	 *
	 * @tparam K The type of the key, which must be convertible to String.
	 * @tparam V The type of the value, which must be either a Value or Ctrls (used for control flags).
	 */
	template<typename K, typename V>
	struct Field {
		K name;
		V value;
	};

	/**
	 * Creates a Field instance with the given name and value.
	 *
	 * This is a convenience function to create a Field object without having to specify the types explicitly.
	 *
	 * @tparam K Type of the key, which must be convertible to String.
	 * @tparam V Type of the value, which must be either a Value or Ctrls.
	 *
	 * @param key The key for the field, which must be convertible to String.
	 * @param value The value for the field, which must be either a Value or Ctrls.
	 *
	 * @return A Field instance containing the provided name and value.
	 */
	template<typename K, typename V>
	static Field<K, V> MakeField(K&& key, V&& value)
	{
		return {std::forward<K>(key), std::forward<V>(value)};
	}

	/**
	 * Encodes a variadic list of arguments into JSON format and writes them to the output stream.
	 *
	 * It accepts a number of arguments of type T, where T can only either be a @c Value, @c Ctrls, or
	 * a @c Field<K, V> type. The arguments are encoded in the order they are provided, and the function
	 * handles both single values and key-value pairs (fields) seamlessly.
	 *
	 * This function is intended to be used by external users to encode JSON data in a more convenient
	 * way without having to manually call the @c EncodeImpl() method for each argument individually.
	 *
	 * @tparam T The type of the first argument to encode.
	 * @tparam Args A variadic template parameter pack for additional arguments to encode.
	 *
	 * @param first The first argument to encode.
	 * @param rest The remaining arguments to encode, if any.
	 */
	template<typename T, typename... Args>
	void Encode(T&& first, Args&&... rest)
	{
		EncodeImpl(std::forward<T>(first));
		/**
		 * If there are more arguments, recursively call EncodeImpl for each of them.
		 *
		 * Note: Putting all the arguments into a e.g. std::tuple<T, Args...> were an option to eliminate the
		 * recursion, but since std::make_tuple(...) makes a copy of the non-rvalue arguments, it would not be
		 * as efficient as the current implementation which uses perfect forwarding to avoid unnecessary copies.
		 */
		if constexpr (sizeof...(rest) > 0) {
			Encode(std::forward<Args>(rest)...);
		}
	}

	void EncodeItem(const Value& item);

private:
	JsonEncoder(nlohmann::detail::output_adapter_t<char> stream, bool pretty)
		: m_Pretty(pretty), m_Indent{0}, m_IndentStr(32, ' '), m_Writer(std::move(stream))
	{}

	void EncodeImpl(Ctrls flag);
	void EncodeImpl(const Value& value);

	enum class CtxType {
		Object, // We've started writing a JSON object, and we expect to see key-value pairs.
		Array, // We've started writing a JSON array, and we expect to see values.
	};

	/**
	 * Encodes a Field (key-value pair) into the currently active JSON object.
	 *
	 * This function is used to encode a single key-value pair, which is typically used to incrementally
	 * build a JSON object by adding fields one by one. Calling this function outside an object context
	 * will result in an exception being thrown.
	 *
	 * @tparam K Type of the key and must be convertible to String.
	 * @tparam V Type of the value and must be either a Value or Ctrls.
	 *
	 * @param field The key-value pair to encode.
	 */
	template<typename K, typename V>
	void EncodeImpl(Field<K, V>&& field)
	{
		if (m_CtxStack.empty() || m_CtxStack.top().type != CtxType::Object) {
			BOOST_THROW_EXCEPTION(std::logic_error("Cannot encode key-value pair outside an object context."));
		}

		writeCommaIfNeededAndIndentStr();
		EncodeValidatedJson(Utility::ValidateUTF8(field.name));
		m_Writer->write_characters(m_Pretty ? ": " : ":", m_Pretty ? 2 : 1);
		EncodeImpl(field.value);
		// Ctrls commands are not actual values, so we don't count them as elements in this context,
		// otherwise we'll end up with an invalid JSON due to an extra comma or newline character.
		if (!std::is_same_v<Ctrls, V>) {
			incrementCtxElements();
		}
	}

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

		if (container->GetLength() == 0) {
			EncodeImpl(EmptyObject);
		} else {
			ObjectLock olock(container);
			auto begin(container->Begin());
			auto end(container->End());
			olock.Unlock();

			EncodeImpl(BeginObject);
			for (auto it(begin); it != end; ++it) {
				EncodeImpl(MakeField(it->first, extractor(it->second)));
			}
			EncodeImpl(EndObject);
		}
	}

	void writeCommaIfNeededAndIndentStr();
	void incrementCtxElements();

	void Indent();
	void UnIndent();

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

	struct Ctx {
		CtxType type; // The current context type (Object, Array).
		unsigned count; // The number of elements in the current context seen so far.
	};
	// The current context stack for nested JSON objects and arrays.
	std::stack<Ctx> m_CtxStack;

	// The output stream adapter for writing JSON data. This can be either a std::ostream or an Asio stream adapter.
	nlohmann::detail::output_adapter_t<char> m_Writer;
};

String JsonEncode(const Value& value, bool pretty_print = false);
void JsonEncode(const Value& value, std::ostream& os, bool prettify = false);
Value JsonDecode(const String& data);

}

#endif /* JSON_H */
