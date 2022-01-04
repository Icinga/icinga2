/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/array.hpp"
#include "base/objectlock.hpp"
#include "base/debug.hpp"
#include "base/primitivetype.hpp"
#include "base/dictionary.hpp"
#include "base/configwriter.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"

using namespace icinga;

template class std::vector<Value>;

REGISTER_PRIMITIVE_TYPE(Array, Object, Array::GetPrototype());

Array::Array(const ArrayData& other)
	: m_Data(other)
{ }

Array::Array(ArrayData&& other)
	: m_Data(std::move(other))
{ }

Array::Array(std::initializer_list<Value> init)
	: m_Data(init)
{ }

/**
 * Restrieves a value from an array.
 *
 * @param index The index.
 * @returns The value.
 */
Value Array::Get(SizeType index) const
{
	ObjectLock olock(this);

	return m_Data.at(index);
}

/**
 * Sets a value in the array.
 *
 * @param index The index.
 * @param value The value.
 * @param overrideFrozen Whether to allow modifying frozen arrays.
 */
void Array::Set(SizeType index, const Value& value, bool overrideFrozen)
{
	ObjectLock olock(this);

	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Value in array must not be modified."));

	m_Data.at(index) = value;
}

/**
 * Sets a value in the array.
 *
 * @param index The index.
 * @param value The value.
 * @param overrideFrozen Whether to allow modifying frozen arrays.
 */
void Array::Set(SizeType index, Value&& value, bool overrideFrozen)
{
	ObjectLock olock(this);

	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Array must not be modified."));

	m_Data.at(index).Swap(value);
}

/**
 * Adds a value to the array.
 *
 * @param value The value.
 * @param overrideFrozen Whether to allow modifying frozen arrays.
 */
void Array::Add(Value value, bool overrideFrozen)
{
	ObjectLock olock(this);

	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Array must not be modified."));

	m_Data.push_back(std::move(value));
}

/**
 * Returns an iterator to the beginning of the array.
 *
 * Note: Caller must hold the object lock while using the iterator.
 *
 * @returns An iterator.
 */
Array::Iterator Array::Begin()
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
Array::Iterator Array::End()
{
	ASSERT(OwnsLock());

	return m_Data.end();
}

/**
 * Returns the number of elements in the array.
 *
 * @returns Number of elements.
 */
size_t Array::GetLength() const
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
 * @param overrideFrozen Whether to allow modifying frozen arrays.
 */
void Array::Insert(SizeType index, Value value, bool overrideFrozen)
{
	ObjectLock olock(this);

	ASSERT(index <= m_Data.size());

	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Array must not be modified."));

	m_Data.insert(m_Data.begin() + index, std::move(value));
}

/**
 * Removes the specified index from the array.
 *
 * @param index The index.
 * @param overrideFrozen Whether to allow modifying frozen arrays.
 */
void Array::Remove(SizeType index, bool overrideFrozen)
{
	ObjectLock olock(this);

	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Array must not be modified."));

	if (index >= m_Data.size())
		BOOST_THROW_EXCEPTION(std::invalid_argument("Index to remove must be within bounds."));

	m_Data.erase(m_Data.begin() + index);
}

/**
 * Removes the item specified by the iterator from the array.
 *
 * @param it The iterator.
 * @param overrideFrozen Whether to allow modifying frozen arrays.
 */
void Array::Remove(Array::Iterator it, bool overrideFrozen)
{
	ASSERT(OwnsLock());

	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Array must not be modified."));

	m_Data.erase(it);
}

void Array::Resize(SizeType newSize, bool overrideFrozen)
{
	ObjectLock olock(this);

	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Array must not be modified."));

	m_Data.resize(newSize);
}

void Array::Clear(bool overrideFrozen)
{
	ObjectLock olock(this);

	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Array must not be modified."));

	m_Data.clear();
}

void Array::Reserve(SizeType newSize, bool overrideFrozen)
{
	ObjectLock olock(this);

	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Array must not be modified."));

	m_Data.reserve(newSize);
}

void Array::CopyTo(const Array::Ptr& dest) const
{
	ObjectLock olock(this);
	ObjectLock xlock(dest);

	if (dest->m_Frozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Array must not be modified."));

	std::copy(m_Data.begin(), m_Data.end(), std::back_inserter(dest->m_Data));
}

/**
 * Makes a shallow copy of an array.
 *
 * @returns a copy of the array.
 */
Array::Ptr Array::ShallowClone() const
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
Object::Ptr Array::Clone() const
{
	ArrayData arr;

	ObjectLock olock(this);
	for (const auto& val : m_Data) {
		arr.push_back(val.Clone());
	}

	return new Array(std::move(arr));
}

Array::Ptr Array::Reverse() const
{
	Array::Ptr result = new Array();

	ObjectLock olock(this);
	ObjectLock xlock(result);

	std::copy(m_Data.rbegin(), m_Data.rend(), std::back_inserter(result->m_Data));

	return result;
}

void Array::Sort(bool overrideFrozen)
{
	ObjectLock olock(this);

	if (m_Frozen && !overrideFrozen)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Array must not be modified."));

	std::sort(m_Data.begin(), m_Data.end());
}

String Array::ToString() const
{
	std::ostringstream msgbuf;
	ConfigWriter::EmitArray(msgbuf, 1, const_cast<Array *>(this));
	return msgbuf.str();
}

Value Array::Join(const Value& separator) const
{
	Value result;
	bool first = true;

	ObjectLock olock(this);

	for (const auto& item : m_Data) {
		if (first) {
			first = false;
		} else {
			result = result + separator;
		}

		result = result + item;
	}

	return result;
}

Array::Ptr Array::Unique() const
{
	std::set<Value> result;

	ObjectLock olock(this);

	for (const auto& item : m_Data) {
		result.insert(item);
	}

	return Array::FromSet(result);
}

void Array::Freeze()
{
	ObjectLock olock(this);
	m_Frozen = true;
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

void Array::SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo)
{
	ObjectLock olock(this);

	int index = Convert::ToLong(field);

	if (index < 0)
		BOOST_THROW_EXCEPTION(ScriptError("Array index '" + Convert::ToString(index) + "' is out of bounds.", debugInfo));

	if (static_cast<size_t>(index) >= GetLength())
		Resize(index + 1, overrideFrozen);

	Set(index, value, overrideFrozen);
}

Array::Iterator icinga::begin(const Array::Ptr& x)
{
	return x->Begin();
}

Array::Iterator icinga::end(const Array::Ptr& x)
{
	return x->End();
}
