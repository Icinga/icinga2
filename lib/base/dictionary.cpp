/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/dictionary.h"
#include "base/objectlock.h"
#include "base/debug.h"
#include <cJSON.h>
#include <boost/tuple/tuple.hpp>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

/**
 * Compares dictionary keys using the less operator.
 */
struct DictionaryKeyLessComparer
{
	/**
	 * Compares two keys.
	 *
	 * @param a The first key.
	 * @param b The second key.
	 * @returns true if the first key is less than the second key, false
	 *		 otherwise
	 */
	bool operator()(const std::pair<String, Value>& a, const char *b)
	{
		return a.first < b;
	}

	/**
	 * Compares two keys.
	 *
	 * @param a The first key.
	 * @param b The second key.
	 * @returns true if the first key is less than the second key, false
	 *		 otherwise
	 */
	bool operator()(const char *a, const std::pair<String, Value>& b)
	{
		return a < b.first;
	}
};

/**
 * Restrieves a value from a dictionary.
 *
 * @param key The key whose value should be retrieved.
 * @returns The value of an empty value if the key was not found.
 */
Value Dictionary::Get(const char *key) const
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	std::map<String, Value>::const_iterator it;

	it = std::lower_bound(m_Data.begin(), m_Data.end(), key, DictionaryKeyLessComparer());

	if (it == m_Data.end() || DictionaryKeyLessComparer()(key, *it))
		return Empty;

	return it->second;
}

/**
 * Retrieves a value from the dictionary.
 *
 * @param key The key whose value should be retrieved.
 * @returns The value or an empty value if the key was not found.
 */
Value Dictionary::Get(const String& key) const
{
	return Get(key.CStr());
}

/**
 * Sets a value in the dictionary.
 *
 * @param key The key.
 * @param value The value.
 */
void Dictionary::Set(const String& key, const Value& value)
{
	if (value.IsEmpty()) {
		Remove(key);
		return;
	}

	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	std::pair<std::map<String, Value>::iterator, bool> ret;
	ret = m_Data.insert(std::make_pair(key, value));
	if (!ret.second)
		ret.first->second = value;
}

/**
 * Returns an iterator to the beginning of the dictionary.
 *
 * Note: Caller must hold the object lock while using the iterator.
 *
 * @returns An iterator.
 */
Dictionary::Iterator Dictionary::Begin(void)
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
Dictionary::Iterator Dictionary::End(void)
{
	ASSERT(OwnsLock());

	return m_Data.end();
}

/**
 * Returns the number of elements in the dictionary.
 *
 * @returns Number of elements.
 */
size_t Dictionary::GetLength(void) const
{
	ASSERT(!OwnsLock());
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
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	return (m_Data.find(key) != m_Data.end());
}

/**
 * Removes the specified key from the dictionary.
 *
 * @param key The key.
 */
void Dictionary::Remove(const String& key)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	Dictionary::Iterator it;
	it = m_Data.find(key);

	if (it == m_Data.end())
		return;

	m_Data.erase(it);
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
 * Makes a shallow copy of a dictionary.
 *
 * @returns a copy of the dictionary.
 */
Dictionary::Ptr Dictionary::ShallowClone(void) const
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	Dictionary::Ptr clone = boost::make_shared<Dictionary>();

	String key;
	Value value;
	BOOST_FOREACH(boost::tie(key, value), m_Data) {
		clone->Set(key, value);
	}

	return clone;
}

/**
 * Converts a JSON object to a dictionary.
 *
 * @param json The JSON object.
 * @returns A dictionary that is equivalent to the JSON object.
 */
Dictionary::Ptr Dictionary::FromJson(cJSON *json)
{
	Dictionary::Ptr dictionary = boost::make_shared<Dictionary>();

	ASSERT(json->type == cJSON_Object);

	for (cJSON *i = json->child; i != NULL; i = i->next) {
		dictionary->Set(i->string, Value::FromJson(i));
	}

	return dictionary;
}

/**
 * Converts this dictionary to a JSON object.
 *
 * @returns A JSON object that is equivalent to the dictionary. Values that
 *	    cannot be represented in JSON are omitted.
 */
cJSON *Dictionary::ToJson(void) const
{
	cJSON *json = cJSON_CreateObject();

	try {
		ObjectLock olock(this);

		String key;
		Value value;
		BOOST_FOREACH(boost::tie(key, value), m_Data) {
			cJSON_AddItemToObject(json, key.CStr(), value.ToJson());
		}
	} catch (...) {
		cJSON_Delete(json);
		throw;
	}

	return json;
}
