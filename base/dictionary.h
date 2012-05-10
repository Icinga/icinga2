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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#ifndef DICTIONARY_H
#define DICTIONARY_H

namespace icinga
{

typedef map<string, Variant>::const_iterator ConstDictionaryIterator;
typedef map<string, Variant>::iterator DictionaryIterator;

struct I2_BASE_API PropertyChangedEventArgs : public EventArgs
{
	string Property;
	Variant OldValue;
	Variant NewValue;
};

class I2_BASE_API Dictionary : public Object
{
private:
	map<string, Variant> m_Data;

public:
	typedef shared_ptr<Dictionary> Ptr;
	typedef weak_ptr<Dictionary> WeakPtr;

	bool GetProperty(string key, Variant *value) const;
	void SetProperty(string key, const Variant& value);

	bool GetPropertyString(string key, string *value);
	void SetPropertyString(string key, const string& value);

	bool GetPropertyInteger(string key, long *value);
	void SetPropertyInteger(string key, long value);

	bool GetPropertyDictionary(string key, Dictionary::Ptr *value);
	void SetPropertyDictionary(string key, const Dictionary::Ptr& value);

	bool GetPropertyObject(string key, Object::Ptr *value);
	void SetPropertyObject(string key, const Object::Ptr& value);

	DictionaryIterator Begin(void);
	DictionaryIterator End(void);

	void AddUnnamedProperty(const Variant& value);
	void AddUnnamedPropertyString(const string& value);
	void AddUnnamedPropertyInteger(long value);
	void AddUnnamedPropertyDictionary(const Dictionary::Ptr& value);
	void AddUnnamedPropertyObject(const Object::Ptr& value);

	long GetLength(void) const;

	Event<PropertyChangedEventArgs> OnPropertyChanged;
};

}

#endif /* DICTIONARY_H */
