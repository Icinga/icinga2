/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/i2-base.hpp"
#include "base/objectlock.hpp"
#include "base/value.hpp"
#include <boost/range/iterator.hpp>
#include <vector>
#include <set>

namespace icinga
{

typedef std::vector<Value> ArrayData;

/**
 * An array of Value items.
 *
 * @ingroup base
 */
class Array final : public Object
{
public:
	DECLARE_OBJECT(Array);

	/**
	 * An iterator that can be used to iterate over array elements.
	 */
	typedef std::vector<Value>::iterator Iterator;

	typedef std::vector<Value>::size_type SizeType;

	Array() = default;
	Array(const ArrayData& other);
	Array(ArrayData&& other);
	Array(std::initializer_list<Value> init);

	Value Get(SizeType index) const;
	void Set(SizeType index, const Value& value, bool overrideFrozen = false);
	void Set(SizeType index, Value&& value, bool overrideFrozen = false);
	void Add(Value value, bool overrideFrozen = false);

	Iterator Begin();
	Iterator End();

	size_t GetLength() const;
	bool Contains(const Value& value) const;

	void Insert(SizeType index, Value value, bool overrideFrozen = false);
	void Remove(SizeType index, bool overrideFrozen = false);
	void Remove(Iterator it, bool overrideFrozen = false);

	void Resize(SizeType newSize, bool overrideFrozen = false);
	void Clear(bool overrideFrozen = false);

	void Reserve(SizeType newSize, bool overrideFrozen = false);

	void CopyTo(const Array::Ptr& dest) const;
	Array::Ptr ShallowClone() const;

	static Object::Ptr GetPrototype();

	template<typename T>
	static Array::Ptr FromVector(const std::vector<T>& v)
	{
		Array::Ptr result = new Array();
		ObjectLock olock(result);
		std::copy(v.begin(), v.end(), std::back_inserter(result->m_Data));
		return result;
	}

	template<typename T>
	std::set<T> ToSet()
	{
		ObjectLock olock(this);
		return std::set<T>(Begin(), End());
	}

	template<typename T>
	static Array::Ptr FromSet(const std::set<T>& v)
	{
		Array::Ptr result = new Array();
		ObjectLock olock(result);
		std::copy(v.begin(), v.end(), std::back_inserter(result->m_Data));
		return result;
	}

	Object::Ptr Clone() const override;

	Array::Ptr Reverse() const;

	void Sort(bool overrideFrozen = false);

	String ToString() const override;
	Value Join(const Value& separator) const;

	Array::Ptr Unique() const;
	void Freeze();

	Value GetFieldByName(const String& field, bool sandboxed, const DebugInfo& debugInfo) const override;
	void SetFieldByName(const String& field, const Value& value, bool overrideFrozen, const DebugInfo& debugInfo) override;

private:
	std::vector<Value> m_Data; /**< The data for the array. */
	bool m_Frozen{false};
};

Array::Iterator begin(const Array::Ptr& x);
Array::Iterator end(const Array::Ptr& x);

}

extern template class std::vector<icinga::Value>;
