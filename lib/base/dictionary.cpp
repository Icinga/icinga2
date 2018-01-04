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

#include "base/dictionary.hpp"
#include "base/objectlock.hpp"
#include "base/debug.hpp"
#include "base/primitivetype.hpp"
#include "base/configwriter.hpp"

using namespace icinga;

template class std::map<String, Value>;

REGISTER_PRIMITIVE_TYPE(Dictionary, Object, Dictionary::GetPrototype());

Dictionary::Dictionary()
{ }

Dictionary::~Dictionary()
{ }

/**
 * Retrieves a value from a dictionary.
 *
 * @param key The key whose value should be retrieved.
 * @returns The value of an empty value if the key was not found.
 */
Value Dictionary::Get(const String& key) const
{
	ObjectLock olock(this);

	auto it = m_Data.find(key);

	if (it == m_Data.end())
		return Empty;

	return it->second;
}

/**
 * Retrieves a value from a dictionary.
 *
 * @param key The key whose value should be retrieved.
 * @param result The value of the dictionary item (only set when the key exists)
 * @returns true if the key exists, false otherwise.
 */
bool Dictionary::Get(const String& key, Value *result) const
{
	ObjectLock olock(this);

	auto it = m_Data.find(key);

	if (it == m_Data.end())
		return false;

	*result = it->second;
	return true;
}

/**
 * Sets a value in the dictionary.
 *
 * @param key The key.
 * @param value The value.
 */
void Dictionary::Set(const String& key, const Value& value)
{
	ObjectLock olock(this);

	m_Data[key] = value;
}

/**
 * Sets a value in the dictionary.
 *
 * @param key The key.
 * @param value The value.
 */
void Dictionary::Set(const String& key, Value&& value)
{
	ObjectLock olock(this);

	m_Data[key] = std::move(value);
}

/**
 * Returns the number of elements in the dictionary.
 *
 * @returns Number of elements.
 */
size_t Dictionary::GetLength() const
{
	ObjectLock olock(this);

	return m_Data.size();
}

/**
 * Checks whether the dictionary contains the specified key.
 *
 * @param key The key.
 * @returns true if the dictionary contains the key, false otherwise.
 */
bool Dictionary::Contains(const String& key) const
{
	ObjectLock olock(this);

	return (m_Data.find(key) != m_Data.end());
}

/**
 * Returns an iterator to the beginning of the dictionary.
 *
 * Note: Caller must hold the object lock while using the iterator.
 *
 * @returns An iterator.
 */
Dictionary::Iterator Dictionary::Begin()
{
	ASSERT(OwnsLock());

	return m_Data.begin();
}

/**
 * Returns an iterator to the end of the dictionary.
 *
 * Note: Caller must hold the object lock while using the iterator.
 *
 * @returns An iterator.
 */
Dictionary::Iterator Dictionary::End()
{
	ASSERT(OwnsLock());

	return m_Data.end();
}

/**
 * Removes the item specified by the iterator from the dictionary.
 *
 * @param it The iterator.
 */
void Dictionary::Remove(Dictionary::Iterator it)
{
	ASSERT(OwnsLock());

	m_Data.erase(it);
}

/**
 * Removes the specified key from the dictionary.
 *
 * @param key The key.
 */
void Dictionary::Remove(const String& key)
{
	ObjectLock olock(this);

	Dictionary::Iterator it;
	it = m_Data.find(key);

	if (it == m_Data.end())
		return;

	m_Data.erase(it);
}

/**
 * Removes all dictionary items.
 */
void Dictionary::Clear()
{
	ObjectLock olock(this);

	m_Data.clear();
}

void Dictionary::CopyTo(const Dictionary::Ptr& dest) const
{
	ObjectLock olock(this);

	for (const Dictionary::Pair& kv : m_Data) {
		dest->Set(kv.first, kv.second);
	}
}

/**
 * Makes a shallow copy of a dictionary.
 *
 * @returns a copy of the dictionary.
 */
Dictionary::Ptr Dictionary::ShallowClone() const
{
	Dictionary::Ptr clone = new Dictionary();
	CopyTo(clone);
	return clone;
}

/**
 * Makes a deep clone of a dictionary
 * and its elements.
 *
 * @returns a copy of the dictionary.
 */
Object::Ptr Dictionary::Clone() const
{
	Dictionary::Ptr dict = new Dictionary();

	ObjectLock olock(this);
	for (const Dictionary::Pair& kv : m_Data) {
		dict->Set(kv.first, kv.second.Clone());
	}

	return dict;
}

/**
 * Returns an array containing all keys
 * which are currently set in this directory.
 *
 * @returns an array of key names
 */
std::vector<String> Dictionary::GetKeys() const
{
	ObjectLock olock(this);

	std::vector<String> keys;

	for (const Dictionary::Pair& kv : m_Data) {
		keys.push_back(kv.first);
	}

	return keys;
}

String Dictionary::ToString() const
{
	std::ostringstream msgbuf;
	ConfigWriter::EmitScope(msgbuf, 1, const_cast<Dictionary *>(this));
	return msgbuf.str();
}

Value Dictionary::GetFieldByName(const String& field, bool, const DebugInfo& debugInfo) const
{
	Value value;

	if (Get(field, &value))
		return value;
	else
		return GetPrototypeField(const_cast<Dictionary *>(this), field, false, debugInfo);
}

void Dictionary::SetFieldByName(const String& field, const Value& value, const DebugInfo&)
{
	Set(field, value);
}

bool Dictionary::HasOwnField(const String& field) const
{
	return Contains(field);
}

bool Dictionary::GetOwnField(const String& field, Value *result) const
{
	return Get(field, result);
}

Dictionary::Iterator icinga::begin(Dictionary::Ptr x)
{
	return x->Begin();
}

Dictionary::Iterator icinga::end(Dictionary::Ptr x)
{
	return x->End();
}

