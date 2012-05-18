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

typedef map<string, Variant>::const_iterator ConstDictionaryIterator;
typedef map<string, Variant>::iterator DictionaryIterator;

/**
 * A container that holds key-value pairs.
 *
 * @ingroup base
 */
class I2_BASE_API Dictionary : public Object
{
private:
	map<string, Variant> m_Data;

public:
	typedef shared_ptr<Dictionary> Ptr;
	typedef weak_ptr<Dictionary> WeakPtr;

	/**
	 * Retrieves a value from the dictionary.
	 *
	 * @param key The key.
	 * @param value Pointer to the value.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	template<typename T>
	bool GetProperty(string key, T *value) const
	{
		ConstDictionaryIterator i = m_Data.find(key);

		if (i == m_Data.end())
			return false;

		*value = static_cast<T>(i->second);

		return true;
	}

	/**
	 * Sets a value in the dictionary.
	 *
	 * @param key The key.
	 * @param value The value.
	 */
	template<typename T>
	void SetProperty(string key, const T& value)
	{
		m_Data[key] = value;
	}

	/**
	 * Adds an unnamed value to the dictionary.
	 *
	 * @param value The value.
	 */
	template<typename T>
	void AddUnnamedProperty(const T& value)
	{
		DictionaryIterator it;
		string key;
		long index = GetLength();
		do {
			stringstream s;
			s << "_" << index;
			index++;

			key = s.str();
			it = m_Data.find(key);
		} while (it != m_Data.end());

		SetProperty(key, value);
	}

	DictionaryIterator Begin(void);
	DictionaryIterator End(void);

	long GetLength(void) const;
};

}

#endif /* DICTIONARY_H */
