/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/json.hpp"
#include "base/array.hpp"

using namespace icinga;

constexpr uint8_t JsonEncoder::m_IndentSize;

/**
 * Emits JSON token(s) based on the provided control flag.
 *
 * This function handles various control flags that dictate how the JSON structure should be encoded,
 * and it manages the context stack to ensure proper nesting of JSON objects and arrays.
 *
 * @param flag The control flag indicating the types of JSON structure to encode.
 */
void JsonEncoder::EncodeImpl(Ctrls flag)
{
	switch (flag) {
		case BeginObject:
			m_CtxStack.push(Ctx{CtxType::Object, 0});
			Indent();
			m_Writer->write_characters(m_Pretty ? "{\n" : "{", m_Pretty ? 2 : 1);
			return;
		case EndObject:
			// Ensure that we are indeed in an object context before closing it.
			ASSERT(!m_CtxStack.empty() && m_CtxStack.top().type == CtxType::Object);
			m_CtxStack.pop();
			UnIndent();
			m_Writer->write_character('}');
			return;
		case BeginArray:
			m_CtxStack.push(Ctx{CtxType::Array, 0});
			Indent();
			m_Writer->write_characters(m_Pretty ? "[\n" : "[", m_Pretty ? 2 : 1);
			return;
		case EndArray:
			// Ensure that we are indeed in an array context before closing it.
			ASSERT(!m_CtxStack.empty() && m_CtxStack.top().type == CtxType::Array);
			m_CtxStack.pop();
			UnIndent();
			m_Writer->write_character(']');
			return;
		case EmptyObject:
			m_Writer->write_characters("{}", 2);
			return;
		case EmptyArray:
			m_Writer->write_characters("[]", 2);
			return;
		case Comma:
			m_Writer->write_characters(",\n", m_Pretty ? 2 : 1);
			return;
		case NewLine:
			m_Writer->write_character('\n');
			return;
		case Flush: {
			while (!m_CtxStack.empty()) {
				if (m_CtxStack.top().type == CtxType::Object) {
					EncodeImpl(EndObject);
				} else if (m_CtxStack.top().type == CtxType::Array) {
					EncodeImpl(EndArray);
				}
				m_CtxStack.pop();
			}
			return;
		}
		default:
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid JSON encoding flag."));
	}
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
			m_Writer->write_characters("null", 4);
			return;
		case ValueBoolean:
			if (value.ToBool()) {
				m_Writer->write_characters("true", 4);
			} else {
				m_Writer->write_characters("false", 5);
			}
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
			if (obj->GetReflectionType() == Namespace::TypeInstance) {
				static constexpr auto extractor = [](const NamespaceValue& v) -> const Value& { return v.Val; };
				EncodeObject(static_pointer_cast<Namespace>(obj), extractor);
			} else if (obj->GetReflectionType() == Dictionary::TypeInstance) {
				static constexpr auto extractor = [](const Value& v) -> const Value& { return v; };
				EncodeObject(static_pointer_cast<Dictionary>(obj), extractor);
			} else if (obj->GetReflectionType() == Array::TypeInstance) {
				if (auto arr(static_pointer_cast<Array>(obj)); arr->GetLength() == 0) {
					EncodeImpl(EmptyArray);
				} else {
					ObjectLock olock(arr);
					auto begin(arr->Begin());
					auto end(arr->End());
					olock.Unlock();

					EncodeImpl(BeginArray);
					for (auto it(begin); it != end; ++it) {
						EncodeItem(*it);
					}
					EncodeImpl(EndArray);
				}
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
 * Encodes an item into the currently open JSON array.
 *
 * This is used to incrementally build a JSON array by adding items to it one by one.
 * Calling this function outside an array context will result in an exception being thrown.
 *
 * @param item The item to be encoded into the JSON array.
 */
void JsonEncoder::EncodeItem(const Value& item)
{
	if (m_CtxStack.empty() || m_CtxStack.top().type != CtxType::Array) {
		BOOST_THROW_EXCEPTION(std::logic_error("Cannot encode item outside an array context."));
	}

	writeCommaIfNeededAndIndentStr();
	EncodeImpl(item);
	incrementCtxElements();
}

void JsonEncoder::writeCommaIfNeededAndIndentStr()
{
	if (!m_CtxStack.empty() && m_CtxStack.top().count > 0) {
		EncodeImpl(Comma);
	}
	if (m_Pretty) {
		m_Writer->write_characters(m_IndentStr.c_str(), m_Indent);
	}
}

void JsonEncoder::incrementCtxElements()
{
	if (!m_CtxStack.empty()) {
		m_CtxStack.top().count++;
	}
}

void JsonEncoder::Indent()
{
	if (m_Pretty) {
		m_Indent += m_IndentSize;
		if (m_IndentStr.size() < m_Indent) {
			m_IndentStr.resize(m_IndentStr.size() * 2, ' ');
		}
	}
}

void JsonEncoder::UnIndent()
{
	if (m_Pretty) {
		m_Indent -= m_IndentSize;
		m_Writer->write_character('\n');
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
	std::ostringstream oss;
	JsonEncode(value, oss, pretty_print);
	return oss.str();
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
