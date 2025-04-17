/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/json.hpp"
#include "base/debug.hpp"
#include "base/namespace.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include <bitset>
#include <boost/exception_ptr.hpp>
#include <cstdint>
#include <json.hpp>
#include <stack>
#include <utility>
#include <vector>

using namespace icinga;

// The following code is a specialization of the nlohmann::adl_serializer<> for our Value type.
// This allows us to serialize Icinga Values directly into JSON objects using the nlohmann::json library.
// The specialization is defined within the nlohmann namespace, which is where the library expects it to be.
namespace nlohmann
{
	template<>
	struct adl_serializer<Value>
	{
		/**
		 * Serializes an Icinga Value into a JSON object.
		 *
		 * @note This function is used by the JSON library to serialize the value.
		 *
		 * @param j The JSON object to serialize the value into.
		 * @param value The value to serialize.
		 */
		static void to_json(json& j, const Value& value)
		{
			switch (value.GetType()) {
				case ValueNumber:
					if (auto ll(static_cast<long long>(value)); ll == value) {
						j = ll;
					} else {
						j = value.Get<double>();
					}
					break;
				case ValueBoolean:
					j = value.ToBool();
					break;
				case ValueString:
					j = Utility::ValidateUTF8(value.Get<String>());
					break;
				case ValueObject:
				{
					const Object::Ptr& obj = value.Get<Object::Ptr>();
					if (obj->GetReflectionType() == Namespace::TypeInstance) {
						ObjectLock olock(obj);
						auto nsJson(json::object());
						for (const auto& [key, value] : static_pointer_cast<Namespace>(obj)) {
							nsJson[Utility::ValidateUTF8(key)] = value.Val;
						}
						j = std::move(nsJson);
					} else if (obj->GetReflectionType() == Dictionary::TypeInstance) {
						ObjectLock olock(obj);
						auto dictJson(json::object());
						for (const auto& [key, value] : static_pointer_cast<Dictionary>(obj)) {
							dictJson[Utility::ValidateUTF8(key)] = value;
						}
						j = std::move(dictJson);
					} else if (obj->GetReflectionType() == Array::TypeInstance) {
						ObjectLock olock(obj);
						auto arrJson(json::array());
						for (const Value& v : static_pointer_cast<Array>(obj)) {
							arrJson.emplace_back(v);
						}
						j = std::move(arrJson);
					} else { // Some other non-serializable object type!
						j = obj->ToString();
					}
					break;
				}
				case ValueEmpty:
					j = nullptr;
					break;
				default:
					VERIFY(!"Invalid variant type.");
			}
		}

		// Deserializing JSON objects into Icinga Values is not supported here.
		// For JSON deserialization, use the JsonSax class instead.
		static void from_json(const json&, Value&)
		{
			throw std::runtime_error("JSON#from_json is not supported for icinga::Value");
		}
	};
} // namespace nlohmann

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

// The number of spaces to use for indentation in pretty-printed JSON.
static constexpr unsigned int l_JsonIndentSize = 4;

String icinga::JsonEncode(const Value& value, bool pretty_print)
{
	nlohmann::json json(value);
	return json.dump(pretty_print ? l_JsonIndentSize : -1, ' ', true);
}

/**
 * Serializes an Icinga Value into a JSON object and writes it to the given output stream.
 *
 * @param value The value to be JSON serialized.
 * @param os The output stream to write the JSON data to.
 * @param pretty_print Whether to pretty print the JSON data.
 */
void icinga::JsonEncode(const Value& value, std::ostream& os, bool pretty_print)
{
	using namespace nlohmann;
	detail::serializer<json> s(detail::output_adapter<char>(os), ' ', json::error_handler_t::strict);
	s.dump(json(value), pretty_print, true, pretty_print ? l_JsonIndentSize : 0);
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
