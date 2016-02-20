/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include "base/i2-base.hpp"
#include "base/object.hpp"
#include "base/value.hpp"
#include <boost/range/iterator.hpp>
#include <map>
#include <vector>

namespace icinga
{

/**
 * A container that holds key-value pairs.
 *
 * @ingroup base
 */
class I2_BASE_API Dictionary : public Object
{
public:
	DECLARE_OBJECT(Dictionary);

	/**
	 * An iterator that can be used to iterate over dictionary elements.
	 */
	typedef std::map<String, Value>::iterator Iterator;

	typedef std::map<String, Value>::size_type SizeType;

	typedef std::pair<String, Value> Pair;

	inline Dictionary(void)
	{ }

	inline ~Dictionary(void)
	{ }

	Value Get(const String& key) const;
	bool Get(const String& key, Value *result) const;
	void Set(const String& key, const Value& value);
	bool Contains(const String& key) const;

	/**
	 * Returns an iterator to the beginning of the dictionary.
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
	 * Returns an iterator to the end of the dictionary.
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

	void Remove(const String& key);

	/**
	 * Removes the item specified by the iterator from the dictionary.
	 *
	 * @param it The iterator.
	 */
	inline void Remove(Iterator it)
	{
		ASSERT(OwnsLock());

		m_Data.erase(it);
	}

	void Clear(void);

	void CopyTo(const Dictionary::Ptr& dest) const;
	Dictionary::Ptr ShallowClone(void) const;

	std::vector<String> GetKeys(void) const;

	static Object::Ptr GetPrototype(void);
	
	virtual Object::Ptr Clone(void) const override;

	virtual String ToString(void) const override;

private:
	std::map<String, Value> m_Data; /**< The data for the dictionary. */
};

inline Dictionary::Iterator range_begin(Dictionary::Ptr x)
{
	return x->Begin();
}

inline Dictionary::Iterator range_end(Dictionary::Ptr x)
{
	return x->End();
}

}

namespace boost
{

template<>
struct range_mutable_iterator<icinga::Dictionary::Ptr>
{
	typedef icinga::Dictionary::Iterator type;
};

template<>
struct range_const_iterator<icinga::Dictionary::Ptr>
{
	typedef icinga::Dictionary::Iterator type;
};

}

#endif /* DICTIONARY_H */
