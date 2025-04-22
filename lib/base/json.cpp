/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/json.hpp"
#include "base/debug.hpp"
#include "base/array.hpp"
#include "base/generator.hpp"
#include <stack>
#include <utility>
#include <vector>

using namespace icinga;

constexpr uint8_t JsonEncoder::m_IndentSize;

JsonEncoder::JsonEncoder(std::string& output, bool prettify)
	: JsonEncoder{nlohmann::detail::output_adapter<char>(output), prettify}
{
}

JsonEncoder::JsonEncoder(std::basic_ostream<char>& stream, bool prettify)
	: JsonEncoder{nlohmann::detail::output_adapter<char>(stream), prettify}
{
}

JsonEncoder::JsonEncoder(nlohmann::detail::output_adapter_t<char> stream, bool prettify)
	: m_Pretty(prettify), m_Indent{0}, m_IndentStr(32, ' '), m_Writer(std::move(stream))
{
}

/**
 * Encodes the given value into JSON and writes it to the configured output stream.
 *
 * This function is specialized for the @c Value type and recursively encodes each
 * and every concrete type it represents.
 *
 * @param value The value to be JSON serialized.
 */
void JsonEncoder::EncodeImpl(const Value& value)
{
	switch (value.GetType()) {
		case ValueEmpty:
			Write("null");
			return;
		case ValueBoolean:
			Write(value.ToBool() ? "true" : "false");
			return;
		case ValueString:
			EncodeValidatedJson(Utility::ValidateUTF8(value.Get<String>()));
			return;
		case ValueNumber:
			if (auto ll(static_cast<long long>(value)); ll == value) {
				EncodeValidatedJson(ll);
			} else {
				EncodeValidatedJson(value.Get<double>());
			}
			return;
		case ValueObject: {
			const Object::Ptr& obj = value.Get<Object::Ptr>();
			const auto& type = obj->GetReflectionType();
			if (type == Namespace::TypeInstance) {
				static constexpr auto extractor = [](const NamespaceValue& v) -> const Value& { return v.Val; };
				EncodeObject(static_pointer_cast<Namespace>(obj), extractor);
			} else if (type == Dictionary::TypeInstance) {
				static constexpr auto extractor = [](const Value& v) -> const Value& { return v; };
				EncodeObject(static_pointer_cast<Dictionary>(obj), extractor);
			} else if (type == Array::TypeInstance) {
				auto arr(static_pointer_cast<Array>(obj));
				BeginContainer('[');
				ObjectLock olock(arr);
				int count = 0;
				for (const auto& item : arr) {
					WriteSeparatorAndIndentStrIfNeeded(count++);
					EncodeImpl(item);
				}
				EndContainer(']', arr->GetLength() == 0);
			} else if (auto gen(dynamic_pointer_cast<ValueGenerator>(obj)); gen) {
				BeginContainer('[');
				bool isEmpty;
				for (int i = 0; true; ++i) {
					auto result = gen->Next();
					if (!result) {
						isEmpty = i == 0;
						break; // Stop when the generator is exhausted.
					}
					WriteSeparatorAndIndentStrIfNeeded(i);
					EncodeImpl(*result);
				}
				EndContainer(']', isEmpty);
			} else {
				// Some other non-serializable object type!
				EncodeValidatedJson(Utility::ValidateUTF8(obj->ToString()));
			}
			return;
		}
		default:
			VERIFY(!"Invalid variant type.");
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
 * @param count The current item count, used to determine if a separator should be written.
 */
void JsonEncoder::WriteSeparatorAndIndentStrIfNeeded(int count) const
{
	if (count > 0) {
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

String icinga::JsonEncode(const Value& value, bool pretty_print)
{
	std::string output;
	JsonEncoder encoder(output, pretty_print);
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
