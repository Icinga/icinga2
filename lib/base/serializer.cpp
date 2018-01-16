/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "base/serializer.hpp"
#include "base/type.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"

using namespace icinga;

static Array::Ptr SerializeArray(const Array::Ptr& input, int attributeTypes)
{
	ArrayData result;

	result.reserve(input->GetLength());

	ObjectLock olock(input);

	for (const Value& value : input) {
		result.emplace_back(Serialize(value, attributeTypes));
	}

	return new Array(std::move(result));
}

static Dictionary::Ptr SerializeDictionary(const Dictionary::Ptr& input, int attributeTypes)
{
	DictionaryData result;

	result.reserve(input->GetLength());

	ObjectLock olock(input);

	for (const Dictionary::Pair& kv : input) {
		result.emplace_back(kv.first, Serialize(kv.second, attributeTypes));
	}

	return new Dictionary(std::move(result));
}

static Object::Ptr SerializeObject(const Object::Ptr& input, int attributeTypes)
{
	Type::Ptr type = input->GetReflectionType();

	if (!type)
		return nullptr;

	DictionaryData fields;
	fields.reserve(type->GetFieldCount() + 1);

	for (int i = 0; i < type->GetFieldCount(); i++) {
		Field field = type->GetFieldInfo(i);

		if (attributeTypes != 0 && (field.Attributes & attributeTypes) == 0)
			continue;

		if (strcmp(field.Name, "type") == 0)
			continue;

		fields.emplace_back(field.Name, Serialize(input->GetField(i), attributeTypes));
	}

	fields.emplace_back("type", type->GetName());

	return new Dictionary(std::move(fields));
}

static Array::Ptr DeserializeArray(const Array::Ptr& input, bool safe_mode, int attributeTypes)
{
	ArrayData result;

	result.reserve(input->GetLength());

	ObjectLock olock(input);

	for (const Value& value : input) {
		result.emplace_back(Deserialize(value, safe_mode, attributeTypes));
	}

	return new Array(std::move(result));
}

static Dictionary::Ptr DeserializeDictionary(const Dictionary::Ptr& input, bool safe_mode, int attributeTypes)
{
	DictionaryData result;

	result.reserve(input->GetLength());

	ObjectLock olock(input);

	for (const Dictionary::Pair& kv : input) {
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
	for (const Dictionary::Pair& kv : input) {
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

Value icinga::Serialize(const Value& value, int attributeTypes)
{
	if (!value.IsObject())
		return value;

	Object::Ptr input = value;

	Array::Ptr array = dynamic_pointer_cast<Array>(input);

	if (array)
		return SerializeArray(array, attributeTypes);

	Dictionary::Ptr dict = dynamic_pointer_cast<Dictionary>(input);

	if (dict)
		return SerializeDictionary(dict, attributeTypes);

	return SerializeObject(input, attributeTypes);
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
