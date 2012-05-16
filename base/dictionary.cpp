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

using namespace icinga;

/**
 * Retrieves a value from the dictionary.
 *
 * @param key The key.
 * @param value Pointer to the value.
 * @returns true if the value was retrieved, false otherwise.
 */
bool Dictionary::GetProperty(string key, Variant *value) const
{
	ConstDictionaryIterator i = m_Data.find(key);

	if (i == m_Data.end())
		return false;

	*value = i->second;
	return true;
}

/**
 * Sets a value in the dictionary.
 *
 * @param key The key.
 * @param value The value.
 */
void Dictionary::SetProperty(string key, const Variant& value)
{
	DictionaryIterator i = m_Data.find(key);

	Variant oldValue;
	if (i != m_Data.end()) {
		oldValue = i->second;
		m_Data.erase(i);
	}

	m_Data[key] = value;
}

/**
 * Retrieves a value from the dictionary.
 *
 * @param key The key.
 * @param value Pointer to the value.
 * @returns true if the value was retrieved, false otherwise.
 */
bool Dictionary::GetProperty(string key, Dictionary::Ptr *value) const
{
	Object::Ptr object;

	if (!GetProperty(key, &object))
		return false;

	Dictionary::Ptr dictionary = dynamic_pointer_cast<Dictionary>(object);
	if (!dictionary)
		throw InvalidArgumentException();

	*value = dictionary;
	return true;
}

/**
 * Returns an iterator to the beginning of the dictionary.
 *
 * @returns An iterator.
 */
DictionaryIterator Dictionary::Begin(void)
{
	return m_Data.begin();
}

/**
 * Returns an iterator to the end of the dictionary.
 *
 * @returns An iterator.
 */
DictionaryIterator Dictionary::End(void)
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
 * Adds an unnamed value to the dictionary.
 *
 * @param value The value.
 */
void Dictionary::AddUnnamedProperty(const Variant& value)
{
	map<string, Variant>::const_iterator it;
	string key;
	long index = GetLength();
	do {
		stringstream s;
		s << "_" << index;
		index++;

		key = s.str();
		it = m_Data.find(key);
	} while (it != m_Data.end());

	m_Data[key] = value;
}
