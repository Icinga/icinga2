/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-base.h"
#include <cJSON.h>

using namespace icinga;

/**
 * Returns an iterator to the beginning of the dictionary.
 *
 * @returns An iterator.
 */
Dictionary::Iterator Dictionary::Begin(void)
{
	return m_Data.begin();
}

/**
 * Returns an iterator to the end of the dictionary.
 *
 * @returns An iterator.
 */
Dictionary::Iterator Dictionary::End(void)
{
	return m_Data.end();
}

/**
 * Returns the number of elements in the dictionary.
 *
 * @returns Number of elements.
 */
long Dictionary::GetLength(void) const
{
	return m_Data.size();
}

/**
 * Checks whether the dictionary contains the specified key.
 *
 * @param key The key.
 * @returns true if the dictionary contains the key, false otherwise.
 */
bool Dictionary::Contains(const string& key) const
{
	return (m_Data.find(key) != m_Data.end());
}

/**
 * Removes the specified key from the dictionary.
 *
 * @param key The key.
 */
void Dictionary::Remove(const string& key)
{
	Dictionary::Iterator it;
	it = m_Data.find(key);

	if (it == m_Data.end())
		return;

	m_Data.erase(it);
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

	if (json->type != cJSON_Object)
		throw_exception(invalid_argument("JSON type must be cJSON_Object."));

	for (cJSON *i = json->child; i != NULL; i = i->next) {
		dictionary->Set(i->string, Variant::FromJson(i));
	}

	return dictionary;
}

/**
 * Converts a dictionary to a JSON object.
 *
 * @param dictionary The dictionary.
 * @returns A JSON object that is equivalent to the dictionary. Values that
 *	    cannot be represented in JSON are omitted.
 */
cJSON *Dictionary::ToJson(void) const
{
	cJSON *json = cJSON_CreateObject();

	try {
		string key;
		Variant value;
		BOOST_FOREACH(tie(key, value), m_Data) {
			cJSON_AddItemToObject(json, key.c_str(), value.ToJson());
		}
	} catch (...) {
		cJSON_Delete(json);
		throw;
	}

	return json;
}
