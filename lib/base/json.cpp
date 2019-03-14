/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/json.hpp"
#include "base/debug.hpp"
#include "base/namespace.hpp"
#include "base/dictionary.hpp"
#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include <boost/exception_ptr.hpp>
#include <yajl/yajl_version.h>
#include <yajl/yajl_gen.h>
#include <yajl/yajl_parse.h>
#include <json.hpp>
#include <stack>
#include <utility>

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
	bool start_object(std::size_t elements) override;
	bool key(string_t& val) override;
	bool end_object() override;
	bool start_array(std::size_t elements) override;
	bool end_array() override;
	bool parse_error(std::size_t position, const std::string& last_token, const nlohmann::detail::exception& ex) override;

	Value GetResult();

private:
	struct Node
	{
		union
		{
			Dictionary *Object;
			Array *Aray;
		};

		bool IsObject;
	};

	Value m_Root;
	std::stack<Node> m_CurrentSubtree;
	String m_CurrentKey;

	void FillCurrentTarget(Value value);
};

static void Encode(yajl_gen handle, const Value& value);

#if YAJL_MAJOR < 2
typedef unsigned int yajl_size;
#else /* YAJL_MAJOR */
typedef size_t yajl_size;
#endif /* YAJL_MAJOR */

static void EncodeNamespace(yajl_gen handle, const Namespace::Ptr& ns)
{
	yajl_gen_map_open(handle);

	ObjectLock olock(ns);
	for (const Namespace::Pair& kv : ns) {
		yajl_gen_string(handle, reinterpret_cast<const unsigned char *>(kv.first.CStr()), kv.first.GetLength());
		Encode(handle, kv.second->Get());
	}

	yajl_gen_map_close(handle);
}

static void EncodeDictionary(yajl_gen handle, const Dictionary::Ptr& dict)
{
	yajl_gen_map_open(handle);

	ObjectLock olock(dict);
	for (const Dictionary::Pair& kv : dict) {
		yajl_gen_string(handle, reinterpret_cast<const unsigned char *>(kv.first.CStr()), kv.first.GetLength());
		Encode(handle, kv.second);
	}

	yajl_gen_map_close(handle);
}

static void EncodeArray(yajl_gen handle, const Array::Ptr& arr)
{
	yajl_gen_array_open(handle);

	ObjectLock olock(arr);
	for (const Value& value : arr) {
		Encode(handle, value);
	}

	yajl_gen_array_close(handle);
}

static void Encode(yajl_gen handle, const Value& value)
{
	switch (value.GetType()) {
		case ValueNumber:
			if (yajl_gen_double(handle, value.Get<double>()) == yajl_gen_invalid_number)
				yajl_gen_double(handle, 0);

			break;
		case ValueBoolean:
			yajl_gen_bool(handle, value.ToBool());

			break;
		case ValueString:
			yajl_gen_string(handle, reinterpret_cast<const unsigned char *>(value.Get<String>().CStr()), value.Get<String>().GetLength());

			break;
		case ValueObject:
			{
				const Object::Ptr& obj = value.Get<Object::Ptr>();
				Namespace::Ptr ns = dynamic_pointer_cast<Namespace>(obj);

				if (ns) {
					EncodeNamespace(handle, ns);
					break;
				}

				Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(obj);

				if (dict) {
					EncodeDictionary(handle, dict);
					break;
				}

				Array::Ptr arr = dynamic_pointer_cast<Array>(obj);

				if (arr) {
					EncodeArray(handle, arr);
					break;
				}
			}

			yajl_gen_null(handle);

			break;
		case ValueEmpty:
			yajl_gen_null(handle);

			break;
		default:
			VERIFY(!"Invalid variant type.");
	}
}

String icinga::JsonEncode(const Value& value, bool pretty_print)
{
#if YAJL_MAJOR < 2
	yajl_gen_config conf = { pretty_print, "" };
	yajl_gen handle = yajl_gen_alloc(&conf, nullptr);
#else /* YAJL_MAJOR */
	yajl_gen handle = yajl_gen_alloc(nullptr);
	if (pretty_print)
		yajl_gen_config(handle, yajl_gen_beautify, 1);
#endif /* YAJL_MAJOR */

	Encode(handle, value);

	const unsigned char *buf;
	yajl_size len;

	yajl_gen_get_buf(handle, &buf, &len);

	String result = String(buf, buf + len);

	yajl_gen_free(handle);

	return result;
}

Value icinga::JsonDecode(const String& data)
{
	JsonSax stateMachine;

	nlohmann::json::sax_parse(data.Begin(), data.End(), &stateMachine);

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
bool JsonSax::start_object(std::size_t)
{
	auto object (new Dictionary());

	FillCurrentTarget(object);

	m_CurrentSubtree.push(Node{{.Object = object}, true});

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

	m_CurrentSubtree.push(Node{{.Aray = array}, false});

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

		if (node.IsObject) {
			node.Object->Set(m_CurrentKey, value);
		} else {
			node.Aray->Add(value);
		}
	}
}
