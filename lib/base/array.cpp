/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/debug.hpp"
#include "base/primitivetype.hpp"
#include "base/dictionary.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_PRIMITIVE_TYPE(Array, Object, Array::GetPrototype());

/**
 * Restrieves a value from an array.
 *
 * @param index The index.
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
 * Checks whether the array contains the specified value.
 *
 * @param value The value.
 * @returns true if the array contains the value, false otherwise.
 */
bool Array::Contains(const Value& value) const
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	return (std::find(m_Data.begin(), m_Data.end(), value) != m_Data.end());
}

/**
 * Insert the given value at the specified index
 *
 * @param index The index
 * @param value The value to add
 */
void Array::Insert(unsigned int index, const Value& value)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	ASSERT(index <= m_Data.size());

	m_Data.insert(m_Data.begin() + index, value);
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

void Array::Resize(size_t new_size)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	m_Data.resize(new_size);
}

void Array::Clear(void)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	m_Data.clear();
}

void Array::Reserve(size_t new_size)
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);

	m_Data.reserve(new_size);
}

void Array::CopyTo(const Array::Ptr& dest) const
{
	ASSERT(!OwnsLock());
	ObjectLock olock(this);
	ObjectLock xlock(dest);

	std::copy(m_Data.begin(), m_Data.end(), std::back_inserter(dest->m_Data));
}

/**
 * Makes a shallow copy of an array.
 *
 * @returns a copy of the array.
 */
Array::Ptr Array::ShallowClone(void) const
{
	Array::Ptr clone = new Array();
	CopyTo(clone);
	return clone;
}

