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

#include "base/array.h"
#include "base/objectlock.h"
#include "base/debug.h"
#include <cJSON.h>
#include <boost/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

/**
 * Restrieves a value from an array.
 *
 * @param index The index..
 * @returns The value.
 */
Value Array::Get(unsigned int index) const
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	return m_Data.at(index);
}

/**
 * Sets a value in the array.
 *
 * @param index The index.
 * @param value The value.
 */
void Array::Set(unsigned int index, const Value& value)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	m_Data.at(index) = value;
}

/**
 * Adds a value to the array.
 *
 * @param value The value.
 */
void Array::Add(const Value& value)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	m_Data.push_back(value);
}

/**
 * Returns an iterator to the beginning of the array.
 *
 * Note: Caller must hold the object lock while using the iterator.
 *
 * @returns An iterator.
 */
Array::Iterator Array::Begin(void)
{
	ASSERT(OwnsLock());

	return m_Data.begin();
}

/**
 * Returns an iterator to the end of the array.
 *
 * Note: Caller must hold the object lock while using the iterator.
 *
 * @returns An iterator.
 */
Array::Iterator Array::End(void)
{
	ASSERT(OwnsLock());

	return m_Data.end();
}

/**
 * Returns the number of elements in the array.
 *
 * @returns Number of elements.
 */
size_t Array::GetLength(void) const
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	return m_Data.size();
}

/**
 * Removes the specified index from the array.
 *
 * @param index The index.
 */
void Array::Remove(unsigned int index)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	m_Data.erase(m_Data.begin() + index);
}

/**
 * Removes the item specified by the iterator from the array.
 *
 * @param it The iterator.
 */
void Array::Remove(Array::Iterator it)
{
	ASSERT(OwnsLock());

	m_Data.erase(it);
}

/**
 * Makes a shallow copy of an array.
 *
 * @returns a copy of the array.
 */
Array::Ptr Array::ShallowClone(void) const
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	Array::Ptr clone = boost::make_shared<Array>();

	std::copy(m_Data.begin(), m_Data.end(), std::back_inserter(clone->m_Data));

	return clone;
}

/**
 * Converts a JSON object to an array.
 *
 * @param json The JSON object.
 * @returns An array that is equivalent to the JSON object.
 */
Array::Ptr Array::FromJson(cJSON *json)
{
	Array::Ptr array = boost::make_shared<Array>();

	ASSERT(json->type == cJSON_Array);

	for (cJSON *i = json->child; i != NULL; i = i->next) {
		array->Add(Value::FromJson(i));
	}

	return array;
}

/**
 * Converts this array to a JSON object.
 *
 * @returns A JSON object that is equivalent to the array. Values that
 *	    cannot be represented in JSON are omitted.
 */
cJSON *Array::ToJson(void) const
{
	cJSON *json = cJSON_CreateArray();

	try {
		ObjectLock olock(this);

		BOOST_FOREACH(const Value& value, m_Data) {
			cJSON_AddItemToArray(json, value.ToJson());
		}
	} catch (...) {
		cJSON_Delete(json);
		throw;
	}

	return json;
}
