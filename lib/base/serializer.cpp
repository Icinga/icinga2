/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/serializer.hpp"
#include "base/type.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/namespace.hpp"
#include <boost/algorithm/string/join.hpp>
#include <deque>

using namespace icinga;

struct SerializeStackEntry
{
	String Name;
	Value Val;
};

CircularReferenceError::CircularReferenceError(String message, std::vector<String> path)
	: m_Message(message), m_Path(path)
{ }

const char *CircularReferenceError::what(void) const throw()
{
	return m_Message.CStr();
}

std::vector<String> CircularReferenceError::GetPath() const
{
	return m_Path;
}

struct SerializeStack
{
	std::deque<SerializeStackEntry> Entries;

	inline void Push(const String& name, const Value& val)
	{
		Object::Ptr obj;

		if (val.IsObject())
			obj = val;

		if (obj) {
			for (const auto& entry : Entries) {
				if (entry.Val == obj) {
					std::vector<String> path;
					for (const auto& entry : Entries)
						path.push_back(entry.Name);
					path.push_back(name);
					BOOST_THROW_EXCEPTION(CircularReferenceError("Cannot serialize object which recursively refers to itself. Attribute path which leads to the cycle: " + boost::algorithm::join(path, " -> "), path));
				}
			}
		}

		Entries.push_back({ name, obj });
	}

	inline void Pop()
	{
		Entries.pop_back();
	}
};

static Value SerializeInternal(const Value& value, int attributeTypes, SerializeStack& stack);

static Array::Ptr SerializeArray(const Array::Ptr& input, int attributeTypes, SerializeStack& stack)
{
	ArrayData result;

	result.reserve(input->GetLength());

	ObjectLock olock(input);

	int index = 0;

	for (const auto& value : input) {
		stack.Push(Convert::ToString(index), value);
		result.emplace_back(SerializeInternal(value, attributeTypes, stack));
		stack.Pop();
		index++;
	}

	return new Array(std::move(result));
}

static Dictionary::Ptr SerializeDictionary(const Dictionary::Ptr& input, int attributeTypes, SerializeStack& stack)
{
	DictionaryData result;

	result.reserve(input->GetLength());

	ObjectLock olock(input);

	for (const auto& kv : input) {
		stack.Push(kv.first, kv.second);
		result.emplace_back(kv.first, SerializeInternal(kv.second, attributeTypes, stack));
		stack.Pop();
	}

	return new Dictionary(std::move(result));
}

static Dictionary::Ptr SerializeNamespace(const Namespace::Ptr& input, int attributeTypes, SerializeStack& stack)
{
	DictionaryData result;

	ObjectLock olock(input);

	for (const auto& kv : input) {
		Value val = kv.second->Get();
		stack.Push(kv.first, val);
		result.emplace_back(kv.first, Serialize(val, attributeTypes));
		stack.Pop();
	}

	return new Dictionary(std::move(result));
}

static Object::Ptr SerializeObject(const Object::Ptr& input, int attributeTypes, SerializeStack& stack)
{
	Type::Ptr type = input->GetReflectionType();

	if (!type)
		return nullptr;

	DictionaryData fields;
	fields.reserve(type->GetFieldCount() + 1);

	ObjectLock olock(input);

	for (int i = 0; i < type->GetFieldCount(); i++) {
		Field field = type->GetFieldInfo(i);

		if (attributeTypes != 0 && (field.Attributes & attributeTypes) == 0)
			continue;

		if (strcmp(field.Name, "type") == 0)
			continue;

		Value value = input->GetField(i);
		stack.Push(field.Name, value);
		fields.emplace_back(field.Name, SerializeInternal(value, attributeTypes, stack));
		stack.Pop();
	}

	fields.emplace_back("type", type->GetName());

	return new Dictionary(std::move(fields));
}

static Array::Ptr DeserializeArray(const Array::Ptr& input, bool safe_mode, int attributeTypes)
{
	ArrayData result;

	result.reserve(input->GetLength());

	ObjectLock olock(input);

	for (const auto& value : input) {
		result.emplace_back(Deserialize(value, safe_mode, attributeTypes));
	}

	return new Array(std::move(result));
}

static Dictionary::Ptr DeserializeDictionary(const Dictionary::Ptr& input, bool safe_mode, int attributeTypes)
{
	DictionaryData result;

	result.reserve(input->GetLength());

	ObjectLock olock(input);

	for (const auto& kv : input) {
		result.emplace_back(kv.first, Deserialize(kv.second, safe_mode, attributeTypes));
	}

	return new Dictionary(std::move(result));
}

static Object::Ptr DeserializeObject(const Object::Ptr& object, const Dictionary::Ptr& input, bool safe_mode, int attributeTypes)
{
	if (!object && safe_mode)
		BOOST_THROW_EXCEPTION(std::runtime_error("Tried to instantiate object while safe mode is enabled."));

	Type::Ptr type;

	if (object)
		type = object->GetReflectionType();
	else
		type = Type::GetByName(input->Get("type"));

	if (!type)
		return object;

	Object::Ptr instance;

	if (object)
		instance = object;
	else
		instance = type->Instantiate(std::vector<Value>());

	ObjectLock olock(input);
	for (const auto& kv : input) {
		if (kv.first.IsEmpty())
			continue;

		int fid = type->GetFieldId(kv.first);

		if (fid < 0)
			continue;

		Field field = type->GetFieldInfo(fid);

		if ((field.Attributes & attributeTypes) == 0)
			continue;

		try {
			instance->SetField(fid, Deserialize(kv.second, safe_mode, attributeTypes), true);
		} catch (const std::exception&) {
			instance->SetField(fid, Empty);
		}
	}

	return instance;
}

static Value SerializeInternal(const Value& value, int attributeTypes, SerializeStack& stack)
{
	if (!value.IsObject())
		return value;

	Object::Ptr input = value;

	Array::Ptr array = dynamic_pointer_cast<Array>(input);

	if (array)
		return SerializeArray(array, attributeTypes, stack);

	Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(input);

	if (dict)
		return SerializeDictionary(dict, attributeTypes, stack);

	Namespace::Ptr ns = dynamic_pointer_cast<Namespace>(input);

	if (ns)
		return SerializeNamespace(ns, attributeTypes, stack);

	return SerializeObject(input, attributeTypes, stack);
}

Value icinga::Serialize(const Value& value, int attributeTypes)
{
	SerializeStack stack;
	return SerializeInternal(value, attributeTypes, stack);
}

Value icinga::Deserialize(const Value& value, bool safe_mode, int attributeTypes)
{
	return Deserialize(nullptr, value, safe_mode, attributeTypes);
}

Value icinga::Deserialize(const Object::Ptr& object, const Value& value, bool safe_mode, int attributeTypes)
{
	if (!value.IsObject())
		return value;

	Object::Ptr input = value;

	Array::Ptr array = dynamic_pointer_cast<Array>(input);

	if (array)
		return DeserializeArray(array, safe_mode, attributeTypes);

	Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(input);

	ASSERT(dict);

	if ((safe_mode && !object) || !dict->Contains("type"))
		return DeserializeDictionary(dict, safe_mode, attributeTypes);
	else
		return DeserializeObject(object, dict, safe_mode, attributeTypes);
}
