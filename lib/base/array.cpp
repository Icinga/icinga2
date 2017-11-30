/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include "base/configwriter.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"

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
	ObjectLock olock(this);

	m_Data.at(index) = value;
}

/**
 * Sets a value in the array.
 *
 * @param index The index.
 * @param value The value.
 */
void Array::Set(unsigned int index, Value&& value)
{
	ObjectLock olock(this);

	m_Data.at(index).Swap(value);
}

/**
 * Adds a value to the array.
 *
 * @param value The value.
 */
void Array::Add(const Value& value)
{
	ObjectLock olock(this);

	m_Data.push_back(value);
}

/**
 * Adds a value to the array.
 *
 * @param value The value.
 */
void Array::Add(Value&& value)
{
	ObjectLock olock(this);

	m_Data.emplace_back(std::move(value));
}

/**
 * Returns the number of elements in the array.
 *
 * @returns Number of elements.
 */
size_t Array::GetLength(void) const
{
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
	ObjectLock olock(this);

	m_Data.resize(new_size);
}

void Array::Clear(void)
{
	ObjectLock olock(this);

	m_Data.clear();
}

void Array::Reserve(size_t new_size)
{
	ObjectLock olock(this);

	m_Data.reserve(new_size);
}

void Array::CopyTo(const Array::Ptr& dest) const
{
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

/**
 * Makes a deep clone of an array
 * and its elements.
 * 
 * @returns a copy of the array.
 */
Object::Ptr Array::Clone(void) const
{
	Array::Ptr arr = new Array();
	
	ObjectLock olock(this);
	for (const Value& val : m_Data) {
		arr->Add(val.Clone());
	}
	
	return arr;
}

Array::Ptr Array::Reverse(void) const
{
	Array::Ptr result = new Array();

	ObjectLock olock(this);
	ObjectLock xlock(result);

	std::copy(m_Data.rbegin(), m_Data.rend(), std::back_inserter(result->m_Data));

	return result;
}

void Array::Sort(void)
{
	ObjectLock olock(this);
	std::sort(m_Data.begin(), m_Data.end());
}

String Array::ToString(void) const
{
	std::ostringstream msgbuf;
	ConfigWriter::EmitArray(msgbuf, 1, const_cast<Array *>(this));
	return msgbuf.str();
}

Value Array::GetFieldByName(const String& field, bool sandboxed, const DebugInfo& debugInfo) const
{
	int index;

	try {
		index = Convert::ToLong(field);
	} catch (...) {
		return Object::GetFieldByName(field, sandboxed, debugInfo);
	}

	ObjectLock olock(this);

	if (index < 0 || static_cast<size_t>(index) >= GetLength())
		BOOST_THROW_EXCEPTION(ScriptError("Array index '" + Convert::ToString(index) + "' is out of bounds.", debugInfo));

	return Get(index);
}

void Array::SetFieldByName(const String& field, const Value& value, const DebugInfo& debugInfo)
{
	ObjectLock olock(this);

	int index = Convert::ToLong(field);

	if (index < 0)
		BOOST_THROW_EXCEPTION(ScriptError("Array index '" + Convert::ToString(index) + "' is out of bounds.", debugInfo));

	if (static_cast<size_t>(index) >= GetLength())
		Resize(index + 1);

	Set(index, value);
}
