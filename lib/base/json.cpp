/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/json.hpp"
#include "base/debug.hpp"
#include "base/dictionary.hpp"
#include "base/namespace.hpp"
#include "base/objectlock.hpp"
#include <boost/numeric/conversion/cast.hpp>
#include <stack>
#include <utility>
#include <vector>

using namespace icinga;

JsonEncoder::JsonEncoder(std::string& output, bool prettify)
	: JsonEncoder{nlohmann::detail::output_adapter<char>(output), prettify}
{
}

JsonEncoder::JsonEncoder(std::basic_ostream<char>& stream, bool prettify)
	: JsonEncoder{nlohmann::detail::output_adapter<char>(stream), prettify}
{
}

JsonEncoder::JsonEncoder(nlohmann::detail::output_adapter_t<char> w, bool prettify)
	: m_IsAsyncWriter{dynamic_cast<AsyncJsonWriter*>(w.get()) != nullptr}, m_Pretty(prettify), m_Writer(std::move(w))
{
}

/**
 * Encodes a single value into JSON and writes it to the underlying output stream.
 *
 * This method is the main entry point for encoding JSON data. It takes a value of any type that can
 * be represented by our @c Value class recursively and encodes it into JSON in an efficient manner.
 * If prettifying is enabled, the JSON output will be formatted with indentation and newlines for better
 * readability, and the final JSON will also be terminated by a newline character.
 *
 * @note If the used output adapter performs asynchronous I/O operations (it's derived from @c AsyncJsonWriter),
 * please provide a @c boost::asio::yield_context object to allow the encoder to flush the output stream in a
 * safe manner. The encoder will try to regularly give the output stream a chance to flush its data when it is
 * safe to do so, but for this to work, there must be a valid yield context provided. Otherwise, the encoder
 * will not attempt to flush the output stream at all, which may lead to huge memory consumption when encoding
 * large JSON objects or arrays.
 *
 * @param value The value to be JSON serialized.
 * @param yc The optional yield context for asynchronous operations. If provided, it allows the encoder
 * to flush the output stream safely when it has not acquired any object lock on the parent containers.
 */
void JsonEncoder::Encode(const Value& value, boost::asio::yield_context* yc)
{
	switch (value.GetType()) {
		case ValueEmpty:
			Write("null");
			break;
		case ValueBoolean:
			Write(value.ToBool() ? "true" : "false");
			break;
		case ValueString:
			EncodeNlohmannJson(Utility::ValidateUTF8(value.Get<String>()));
			break;
		case ValueNumber:
			EncodeNumber(value.Get<double>());
			break;
		case ValueObject: {
			const auto& obj = value.Get<Object::Ptr>();
			const auto& type = obj->GetReflectionType();
			if (type == Namespace::TypeInstance) {
				static constexpr auto extractor = [](const NamespaceValue& v) -> const Value& { return v.Val; };
				EncodeObject(static_pointer_cast<Namespace>(obj), extractor, yc);
			} else if (type == Dictionary::TypeInstance) {
				static constexpr auto extractor = [](const Value& v) -> const Value& { return v; };
				EncodeObject(static_pointer_cast<Dictionary>(obj), extractor, yc);
			} else if (type == Array::TypeInstance) {
				EncodeArray(static_pointer_cast<Array>(obj), yc);
			} else if (auto gen(dynamic_pointer_cast<ValueGenerator>(obj)); gen) {
				EncodeValueGenerator(gen, yc);
			} else {
				// Some other non-serializable object type!
				EncodeNlohmannJson(Utility::ValidateUTF8(obj->ToString()));
			}
			break;
		}
		default:
			VERIFY(!"Invalid variant type.");
	}

	// If we are at the top level of the JSON object and prettifying is enabled, we need to end
	// the JSON with a newline character to ensure that the output is properly formatted.
	if (m_Indent == 0 && m_Pretty) {
		Write("\n");
	}
}

/**
 * Encodes an Array object into JSON and writes it to the output stream.
 *
 * @param array The Array object to be serialized into JSON.
 * @param yc The optional yield context for asynchronous operations. If provided, it allows the encoder
 * to flush the output stream safely when it has not acquired any object lock.
 */
void JsonEncoder::EncodeArray(const Array::Ptr& array, boost::asio::yield_context* yc)
{
	BeginContainer('[');
	auto olock = array->LockIfRequired();
	if (olock) {
		yc = nullptr; // We've acquired an object lock, never allow asynchronous operations.
	}

	bool isEmpty = true;
	for (const auto& item : array) {
		WriteSeparatorAndIndentStrIfNeeded(!isEmpty);
		isEmpty = false;
		Encode(item, yc);
		FlushIfSafe(yc);
	}
	EndContainer(']', isEmpty);
}

/**
 * Encodes a ValueGenerator object into JSON and writes it to the output stream.
 *
 * This will iterate through the generator, encoding each value it produces until it is exhausted.
 *
 * @param generator The ValueGenerator object to be serialized into JSON.
 * @param yc The optional yield context for asynchronous operations. If provided, it allows the encoder
 * to flush the output stream safely when it has not acquired any object lock on the parent containers.
 */
void JsonEncoder::EncodeValueGenerator(const ValueGenerator::Ptr& generator, boost::asio::yield_context* yc)
{
	BeginContainer('[');
	bool isEmpty = true;
	while (auto result = generator->Next()) {
		WriteSeparatorAndIndentStrIfNeeded(!isEmpty);
		isEmpty = false;
		Encode(*result, yc);
		FlushIfSafe(yc);
	}
	EndContainer(']', isEmpty);
}

/**
 * Encodes an Icinga 2 object (Namespace or Dictionary) into JSON and writes it to @c m_Writer.
 *
 * @tparam Iterable Type of the container (Namespace or Dictionary).
 * @tparam ValExtractor Type of the value extractor function used to extract values from the container's iterator.
 *
 * @param container The container to JSON serialize.
 * @param extractor The value extractor function used to extract values from the container's iterator.
 * @param yc The optional yield context for asynchronous operations. It will only be set when the encoder
 * has not acquired any object lock on the parent containers, allowing safe asynchronous operations.
 */
template<typename Iterable, typename ValExtractor>
void JsonEncoder::EncodeObject(const Iterable& container, const ValExtractor& extractor, boost::asio::yield_context* yc)
{
	static_assert(std::is_same_v<Iterable, Namespace::Ptr> || std::is_same_v<Iterable, Dictionary::Ptr>,
		"Container must be a Namespace or Dictionary");

	BeginContainer('{');
	auto olock = container->LockIfRequired();
	if (olock) {
		yc = nullptr; // We've acquired an object lock, never allow asynchronous operations.
	}

	bool isEmpty = true;
	for (const auto& [key, val] : container) {
		WriteSeparatorAndIndentStrIfNeeded(!isEmpty);
		isEmpty = false;

		EncodeNlohmannJson(Utility::ValidateUTF8(key));
		Write(m_Pretty ? ": " : ":");

		Encode(extractor(val), yc);
		FlushIfSafe(yc);
	}
	EndContainer('}', isEmpty);
}

/**
 * Dumps a nlohmann::json object to the output stream using the serializer.
 *
 * This function uses the @c nlohmann::detail::serializer to dump the provided @c nlohmann::json
 * object to the output stream managed by the @c JsonEncoder.
 *
 * @param json The nlohmann::json object to encode.
 */
void JsonEncoder::EncodeNlohmannJson(const nlohmann::json& json) const
{
	nlohmann::detail::serializer<nlohmann::json> s(m_Writer, ' ', nlohmann::json::error_handler_t::strict);
	s.dump(json, m_Pretty, true, 0, 0);
}

/**
 * Encodes a double value into JSON format and writes it to the output stream.
 *
 * This function checks if the double value can be safely cast to an integer or unsigned integer type
 * without loss of precision. If it can, it will serialize it as such; otherwise, it will serialize
 * it as a double. This is particularly useful for ensuring that values like 0.0 are serialized as 0,
 * which can be important for compatibility with clients like Icinga DB that expect integers in such cases.
 *
 * @param value The double value to encode as JSON.
 */
void JsonEncoder::EncodeNumber(double value) const
{
	try {
		if (value < 0) {
			if (auto ll(boost::numeric_cast<nlohmann::json::number_integer_t>(value)); ll == value) {
				EncodeNlohmannJson(ll);
				return;
			}
		} else if (auto ull(boost::numeric_cast<nlohmann::json::number_unsigned_t>(value)); ull == value) {
			EncodeNlohmannJson(ull);
			return;
		}
		// If we reach this point, the value cannot be safely cast to a signed or unsigned integer
		// type because it would otherwise lose its precision. If the value was just too large to fit
		// into the above types, then boost will throw an exception and end up in the below catch block.
		// So, in either case, serialize the number as-is without any casting.
	} catch (const boost::bad_numeric_cast&) {}

	EncodeNlohmannJson(value);
}

/**
 * Flushes the output stream if it is safe to do so.
 *
 * Safe flushing means that it only performs the flush operation if the @c JsonEncoder has not acquired
 * any object lock so far. This is to ensure that the stream can safely perform asynchronous operations
 * without risking undefined behaviour due to coroutines being suspended while the stream is being flushed.
 *
 * When the @c yc parameter is provided, it indicates that it's safe to perform asynchronous operations,
 * and the function will attempt to flush if the writer is an instance of @c AsyncJsonWriter.
 *
 * @param yc The yield context to use for asynchronous operations.
 */
void JsonEncoder::FlushIfSafe(boost::asio::yield_context* yc) const
{
	if (yc && m_IsAsyncWriter) {
		// The m_IsAsyncWriter flag is a constant, and it will never change, so we can safely static
		// cast the m_Writer to AsyncJsonWriter without any additional checks as it is guaranteed
		// to be an instance of AsyncJsonWriter when m_IsAsyncWriter is true.
		auto ajw(static_cast<AsyncJsonWriter*>(m_Writer.get()));
		ajw->Flush(*yc);
	}
}

/**
 * Writes a string to the underlying output stream.
 *
 * This function writes the provided string view directly to the output stream without any additional formatting.
 *
 * @param sv The string view to write to the output stream.
 */
void JsonEncoder::Write(const std::string_view& sv) const
{
	m_Writer->write_characters(sv.data(), sv.size());
}

/**
 * Begins a JSON container (object or array) by writing the opening character and adjusting the
 * indentation level if pretty-printing is enabled.
 *
 * @param openChar The character that opens the container (either '{' for objects or '[' for arrays).
 */
void JsonEncoder::BeginContainer(char openChar)
{
	if (m_Pretty) {
		m_Indent += m_IndentSize;
		if (m_IndentStr.size() < m_Indent) {
			m_IndentStr.resize(m_IndentStr.size() * 2, ' ');
		}
	}
	m_Writer->write_character(openChar);
}

/**
 * Ends a JSON container (object or array) by writing the closing character and adjusting the
 * indentation level if pretty-printing is enabled.
 *
 * @param closeChar The character that closes the container (either '}' for objects or ']' for arrays).
 * @param isContainerEmpty Whether the container is empty, used to determine if a newline should be written.
 */
void JsonEncoder::EndContainer(char closeChar, bool isContainerEmpty)
{
	if (m_Pretty) {
		ASSERT(m_Indent >= m_IndentSize); // Ensure we don't underflow the indent size.
		m_Indent -= m_IndentSize;
		if (!isContainerEmpty) {
			Write("\n");
			m_Writer->write_characters(m_IndentStr.c_str(), m_Indent);
		}
	}
	m_Writer->write_character(closeChar);
}

/**
 * Writes a separator (comma) and an indentation string if pretty-printing is enabled.
 *
 * This function is used to separate items in a JSON array or object and to maintain the correct indentation level.
 *
 * @param emitComma Whether to emit a comma. This is typically true for all but the first item in a container.
 */
void JsonEncoder::WriteSeparatorAndIndentStrIfNeeded(bool emitComma) const
{
	if (emitComma) {
		Write(",");
	}
	if (m_Pretty) {
		Write("\n");
		m_Writer->write_characters(m_IndentStr.c_str(), m_Indent);
	}
}

class JsonSax : public nlohmann::json_sax<nlohmann::json>
{
public:
	bool null() override;
	bool boolean(bool val) override;
	bool number_integer(number_integer_t val) override;
	bool number_unsigned(number_unsigned_t val) override;
	bool number_float(number_float_t val, const string_t& s) override;
	bool string(string_t& val) override;
	bool binary(binary_t& val) override;
	bool start_object(std::size_t elements) override;
	bool key(string_t& val) override;
	bool end_object() override;
	bool start_array(std::size_t elements) override;
	bool end_array() override;
	bool parse_error(std::size_t position, const std::string& last_token, const nlohmann::detail::exception& ex) override;

	Value GetResult();

private:
	Value m_Root;
	std::stack<std::pair<Dictionary*, Array*>> m_CurrentSubtree;
	String m_CurrentKey;

	void FillCurrentTarget(Value value);
};

String icinga::JsonEncode(const Value& value, bool prettify)
{
	std::string output;
	JsonEncoder encoder(output, prettify);
	encoder.Encode(value);
	return String(std::move(output));
}

/**
 * Serializes an Icinga Value into a JSON object and writes it to the given output stream.
 *
 * @param value The value to be JSON serialized.
 * @param os The output stream to write the JSON data to.
 * @param prettify Whether to pretty print the serialized JSON.
 */
void icinga::JsonEncode(const Value& value, std::ostream& os, bool prettify)
{
	JsonEncoder encoder(os, prettify);
	encoder.Encode(value);
}

Value icinga::JsonDecode(const String& data)
{
	String sanitized (Utility::ValidateUTF8(data));

	JsonSax stateMachine;

	nlohmann::json::sax_parse(sanitized.Begin(), sanitized.End(), &stateMachine);

	return stateMachine.GetResult();
}

inline
bool JsonSax::null()
{
	FillCurrentTarget(Value());

	return true;
}

inline
bool JsonSax::boolean(bool val)
{
	FillCurrentTarget(val);

	return true;
}

inline
bool JsonSax::number_integer(JsonSax::number_integer_t val)
{
	FillCurrentTarget((double)val);

	return true;
}

inline
bool JsonSax::number_unsigned(JsonSax::number_unsigned_t val)
{
	FillCurrentTarget((double)val);

	return true;
}

inline
bool JsonSax::number_float(JsonSax::number_float_t val, const JsonSax::string_t&)
{
	FillCurrentTarget((double)val);

	return true;
}

inline
bool JsonSax::string(JsonSax::string_t& val)
{
	FillCurrentTarget(String(std::move(val)));

	return true;
}

inline
bool JsonSax::binary(JsonSax::binary_t& val)
{
	FillCurrentTarget(String(val.begin(), val.end()));

	return true;
}

inline
bool JsonSax::start_object(std::size_t)
{
	auto object (new Dictionary());

	FillCurrentTarget(object);

	m_CurrentSubtree.push({object, nullptr});

	return true;
}

inline
bool JsonSax::key(JsonSax::string_t& val)
{
	m_CurrentKey = String(std::move(val));

	return true;
}

inline
bool JsonSax::end_object()
{
	m_CurrentSubtree.pop();
	m_CurrentKey = String();

	return true;
}

inline
bool JsonSax::start_array(std::size_t)
{
	auto array (new Array());

	FillCurrentTarget(array);

	m_CurrentSubtree.push({nullptr, array});

	return true;
}

inline
bool JsonSax::end_array()
{
	m_CurrentSubtree.pop();

	return true;
}

inline
bool JsonSax::parse_error(std::size_t, const std::string&, const nlohmann::detail::exception& ex)
{
	throw std::invalid_argument(ex.what());
}

inline
Value JsonSax::GetResult()
{
	return m_Root;
}

inline
void JsonSax::FillCurrentTarget(Value value)
{
	if (m_CurrentSubtree.empty()) {
		m_Root = value;
	} else {
		auto& node (m_CurrentSubtree.top());

		if (node.first) {
			node.first->Set(m_CurrentKey, value);
		} else {
			node.second->Add(value);
		}
	}
}
