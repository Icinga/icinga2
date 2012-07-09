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

#ifndef DICTIONARY_H
#define DICTIONARY_H

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
	typedef shared_ptr<Dictionary> Ptr;
	typedef weak_ptr<Dictionary> WeakPtr;

	typedef map<string, Variant>::const_iterator ConstIterator;
	typedef map<string, Variant>::iterator Iterator;

	/**
	 * Retrieves a value from the dictionary.
	 *
	 * @param key The key.
	 * @param[out] value Pointer to the value.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	template<typename T>
	bool Get(const string& key, T *value) const
	{
		ConstIterator i = m_Data.find(key);

		if (i == m_Data.end())
			return false;

		*value = static_cast<T>(i->second);

		return true;
	}

	/**
	 * Retrieves a value from the dictionary.
	 *
	 * @param key The key.
	 * @param[out] value Pointer to the value.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	bool Get(const string& key, Dictionary::Ptr *value)
	{
		Object::Ptr object;

		if (!Get(key, &object))
			return false;

		*value = dynamic_pointer_cast<Dictionary>(object);
		if (!*value)
			throw runtime_error("Object is not a dictionary.");

		return true;
	}

	/**
	 * Sets a value in the dictionary.
	 *
	 * @param key The key.
	 * @param value The value.
	 */
	template<typename T>
	void Set(const string& key, const T& value)
	{
		pair<typename map<string, Variant>::iterator, bool> ret;
		ret = m_Data.insert(make_pair(key, value));
		if (!ret.second)
			ret.first->second = value;
	}

	/**
	 * Adds an unnamed value to the dictionary.
	 *
	 * @param value The value.
	 */
	template<typename T>
	void Add(const T& value)
	{
		Iterator it;
		string key;
		long index = GetLength();
		do {
			stringstream s;
			s << "_" << index;
			index++;

			key = s.str();
			it = m_Data.find(key);
		} while (it != m_Data.end());

		Set(key, value);
	}

	bool Contains(const string& key) const;

	Iterator Begin(void);
	Iterator End(void);

	long GetLength(void) const;

	void Remove(const string& key);

private:
	map<string, Variant> m_Data;
};

}

#endif /* DICTIONARY_H */
