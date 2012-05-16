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
 */
class I2_BASE_API Dictionary : public Object
{
private:
	map<string, Variant> m_Data;

public:
	typedef shared_ptr<Dictionary> Ptr;
	typedef weak_ptr<Dictionary> WeakPtr;

	bool GetProperty(string key, Variant *value) const;
	void SetProperty(string key, const Variant& value);

	template<typename T>
	bool GetProperty(string key, T *value) const
	{
		Variant data;

		if (!GetProperty(key, &data))
			return false;

		*value = data;

		return true;
	}

	bool GetProperty(string key, Dictionary::Ptr *value) const;

	DictionaryIterator Begin(void);
	DictionaryIterator End(void);

	void AddUnnamedProperty(const Variant& value);

	long GetLength(void) const;
};

}

#endif /* DICTIONARY_H */
