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

#ifndef ARRAY_H
#define ARRAY_H

#include "base/i2-base.hpp"
#include "base/objectlock.hpp"
#include "base/value.hpp"
#include <boost/range/iterator.hpp>
#include <vector>
#include <set>

namespace icinga
{

/**
 * An array of Value items.
 *
 * @ingroup base
 */
class I2_BASE_API Array : public Object
{
public:
	DECLARE_OBJECT(Array);

	/**
	 * An iterator that can be used to iterate over array elements.
	 */
	typedef std::vector<Value>::iterator Iterator;

	typedef std::vector<Value>::size_type SizeType;

	inline Array(void)
	{ }

	inline ~Array(void)
	{ }

	Value Get(unsigned int index) const;
	void Set(unsigned int index, const Value& value);
	void Add(const Value& value);

	/**
	 * Returns an iterator to the beginning of the array.
	 *
	 * Note: Caller must hold the object lock while using the iterator.
	 *
	 * @returns An iterator.
	 */
	inline Iterator Begin(void)
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
	inline Iterator End(void)
	{
		ASSERT(OwnsLock());

		return m_Data.end();
	}

	size_t GetLength(void) const;
	bool Contains(const Value& value) const;

	void Insert(unsigned int index, const Value& value);
	void Remove(unsigned int index);
	void Remove(Iterator it);

	void Resize(size_t new_size);
	void Clear(void);

	void Reserve(size_t new_size);

	void CopyTo(const Array::Ptr& dest) const;
	Array::Ptr ShallowClone(void) const;

	static Object::Ptr GetPrototype(void);

	template<typename T>
	static Array::Ptr FromVector(const std::vector<T>& v)
	{
		Array::Ptr result = new Array();
		ObjectLock olock(result);
		std::copy(v.begin(), v.end(), std::back_inserter(result->m_Data));
		return result;
	}

	template<typename T>
	std::set<T> ToSet(void)
	{
		ObjectLock olock(this);
		return std::set<T>(Begin(), End());
	}

	virtual Object::Ptr Clone(void) const override;

	Array::Ptr Reverse(void) const;

	virtual String ToString(void) const override;

private:
	std::vector<Value> m_Data; /**< The data for the array. */
};

inline Array::Iterator range_begin(Array::Ptr x)
{
	return x->Begin();
}

inline Array::Iterator range_end(Array::Ptr x)
{
	return x->End();
}

}

namespace boost
{

template<>
struct range_mutable_iterator<icinga::Array::Ptr>
{
	typedef icinga::Array::Iterator type;
};

template<>
struct range_const_iterator<icinga::Array::Ptr>
{
	typedef icinga::Array::Iterator type;
};

}

#endif /* ARRAY_H */
