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

	typedef map<String, Value>::iterator Iterator;

	Value Get(const char *key) const;
	Value Get(const String& key) const;
	void Set(const String& key, const Value& value);
	String Add(const Value& value);
	bool Contains(const String& key) const;

	Iterator Begin(void);
	Iterator End(void);

	size_t GetLength(void) const;

	void Remove(const String& key);
	void Remove(Iterator it);

	static Dictionary::Ptr FromJson(cJSON *json);
	cJSON *ToJson(void) const;

private:
	map<String, Value> m_Data;
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
