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

const char l_Null[] = "null";
const char l_False[] = "false";
const char l_True[] = "true";
const char l_Indent[] = "    ";

// https://github.com/nlohmann/json/issues/1512
template<bool prettyPrint>
class JsonEncoder
{
public:
	void Null();
	void Boolean(bool value);
	void NumberFloat(double value);
	void Strng(String value);
	void StartObject();
	void Key(String value);
	void EndObject();
	void StartArray();
	void EndArray();

	String GetResult();

private:
	std::vector<char> m_Result;
	String m_CurrentKey;
	std::stack<std::bitset<2>> m_CurrentSubtree;

	void AppendChar(char c);

	template<class Iterator>
	void AppendChars(Iterator begin, Iterator end);

	void AppendJson(nlohmann::json json);

	void BeforeItem();

	void FinishContainer(char terminator);
};

template<bool prettyPrint>
void Encode(JsonEncoder<prettyPrint>& stateMachine, const Value& value);

template<bool prettyPrint>
inline
void EncodeNamespace(JsonEncoder<prettyPrint>& stateMachine, const Namespace::Ptr& ns)
{
	stateMachine.StartObject();

	ObjectLock olock(ns);
	for (const Namespace::Pair& kv : ns) {
		stateMachine.Key(Utility::ValidateUTF8(kv.first));
		Encode(stateMachine, kv.second->Get());
	}

	stateMachine.EndObject();
}

template<bool prettyPrint>
inline
void EncodeDictionary(JsonEncoder<prettyPrint>& stateMachine, const Dictionary::Ptr& dict)
{
	stateMachine.StartObject();

	ObjectLock olock(dict);
	for (const Dictionary::Pair& kv : dict) {
		stateMachine.Key(Utility::ValidateUTF8(kv.first));
		Encode(stateMachine, kv.second);
	}

	stateMachine.EndObject();
}

template<bool prettyPrint>
inline
void EncodeArray(JsonEncoder<prettyPrint>& stateMachine, const Array::Ptr& arr)
{
	stateMachine.StartArray();

	ObjectLock olock(arr);
	for (const Value& value : arr) {
		Encode(stateMachine, value);
	}

	stateMachine.EndArray();
}

template<bool prettyPrint>
void Encode(JsonEncoder<prettyPrint>& stateMachine, const Value& value)
{
	switch (value.GetType()) {
		case ValueNumber:
			stateMachine.NumberFloat(value.Get<double>());
			break;

		case ValueBoolean:
			stateMachine.Boolean(value.ToBool());
			break;

		case ValueString:
			stateMachine.Strng(Utility::ValidateUTF8(value.Get<String>()));
			break;

		case ValueObject:
			{
				const Object::Ptr& obj = value.Get<Object::Ptr>();

				{
					Namespace::Ptr ns = dynamic_pointer_cast<Namespace>(obj);
					if (ns) {
						EncodeNamespace(stateMachine, ns);
						break;
					}
				}

				{
					Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(obj);
					if (dict) {
						EncodeDictionary(stateMachine, dict);
						break;
					}
				}

				{
					Array::Ptr arr = dynamic_pointer_cast<Array>(obj);
					if (arr) {
						EncodeArray(stateMachine, arr);
						break;
					}
				}

				// obj is most likely a function => "Object of type 'Function'"
				Encode(stateMachine, obj->ToString());
				break;
			}

		case ValueEmpty:
			stateMachine.Null();
			break;

		default:
			VERIFY(!"Invalid variant type.");
	}
}

String icinga::JsonEncode(const Value& value, bool pretty_print)
{
	if (pretty_print) {
		JsonEncoder<true> stateMachine;

		Encode(stateMachine, value);

		return stateMachine.GetResult() + "\n";
	} else {
		JsonEncoder<false> stateMachine;

		Encode(stateMachine, value);

		return stateMachine.GetResult();
	}
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

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::Null()
{
	BeforeItem();
	AppendChars((const char*)l_Null, (const char*)l_Null + 4);
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::Boolean(bool value)
{
	BeforeItem();

	if (value) {
		AppendChars((const char*)l_True, (const char*)l_True + 4);
	} else {
		AppendChars((const char*)l_False, (const char*)l_False + 5);
	}
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::NumberFloat(double value)
{
	BeforeItem();

	// Make sure 0.0 is serialized as 0, so e.g. Icinga DB can parse it as int.
	if (value < 0) {
		long long i = value;

		if (i == value) {
			AppendJson(i);
		} else {
			AppendJson(value);
		}
	} else {
		unsigned long long i = value;

		if (i == value) {
			AppendJson(i);
		} else {
			AppendJson(value);
		}
	}
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::Strng(String value)
{
	BeforeItem();
	AppendJson(std::move(value));
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::StartObject()
{
	BeforeItem();
	AppendChar('{');

	m_CurrentSubtree.push(2);
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::Key(String value)
{
	m_CurrentKey = std::move(value);
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::EndObject()
{
	FinishContainer('}');
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::StartArray()
{
	BeforeItem();
	AppendChar('[');

	m_CurrentSubtree.push(0);
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::EndArray()
{
	FinishContainer(']');
}

template<bool prettyPrint>
inline
String JsonEncoder<prettyPrint>::GetResult()
{
	return String(m_Result.begin(), m_Result.end());
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::AppendChar(char c)
{
	m_Result.emplace_back(c);
}

template<bool prettyPrint>
template<class Iterator>
inline
void JsonEncoder<prettyPrint>::AppendChars(Iterator begin, Iterator end)
{
	m_Result.insert(m_Result.end(), begin, end);
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::AppendJson(nlohmann::json json)
{
	nlohmann::detail::serializer<nlohmann::json>(nlohmann::detail::output_adapter<char>(m_Result), ' ').dump(std::move(json), prettyPrint, true, 0);
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::BeforeItem()
{
	if (!m_CurrentSubtree.empty()) {
		auto& node (m_CurrentSubtree.top());

		if (node[0]) {
			AppendChar(',');
		} else {
			node[0] = true;
		}

		if (prettyPrint) {
			AppendChar('\n');

			for (auto i (m_CurrentSubtree.size()); i; --i) {
				AppendChars((const char*)l_Indent, (const char*)l_Indent + 4);
			}
		}

		if (node[1]) {
			AppendJson(std::move(m_CurrentKey));
			AppendChar(':');

			if (prettyPrint) {
				AppendChar(' ');
			}
		}
	}
}

template<bool prettyPrint>
inline
void JsonEncoder<prettyPrint>::FinishContainer(char terminator)
{
	if (prettyPrint && m_CurrentSubtree.top()[0]) {
		AppendChar('\n');

		for (auto i (m_CurrentSubtree.size() - 1u); i; --i) {
			AppendChars((const char*)l_Indent, (const char*)l_Indent + 4);
		}
	}

	AppendChar(terminator);

	m_CurrentSubtree.pop();
}
