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

#ifndef MESSAGEPART_H
#define MESSAGEPART_H

struct cJSON;

namespace icinga
{

typedef ::cJSON json_t;

/**
 * A part of an RPC message.
 *
 * @ingroup remoting
 */
class I2_REMOTING_API MessagePart
{
public:
	MessagePart(void);
	MessagePart(const Dictionary::Ptr& dictionary);
	MessagePart(const MessagePart& message);

	Dictionary::Ptr GetDictionary(void) const;

	/**
	 * Retrieves a property's value.
	 *
	 * @param key The name of the property.
	 * @param[out] value The value.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	template<typename T>
	bool Get(String key, T *value) const
	{
		Value v = GetDictionary()->Get(key);

		if (v.IsEmpty())
			return false;

		*value = static_cast<T>(v);
		return true;
	}

	/**
	 * Sets a property's value.
	 *
	 * @param key The name of the property.
	 * @param value The value.
	 */
	template<typename T>
	void Set(String key, const T& value)
	{
		GetDictionary()->Set(key, value);
	}

	bool Get(String key, MessagePart *value) const;
	void Set(String key, const MessagePart& value);

	bool Contains(const String& key) const;

	Dictionary::Iterator Begin(void);
	Dictionary::Iterator End(void);

private:
	Dictionary::Ptr m_Dictionary;
};

}

#endif /* MESSAGEPART_H */
