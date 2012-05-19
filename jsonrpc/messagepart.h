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
 * @ingroup jsonrpc
 */
class I2_JSONRPC_API MessagePart
{
private:
	Dictionary::Ptr m_Dictionary;

	static Dictionary::Ptr GetDictionaryFromJson(json_t *json);
	static json_t *GetJsonFromDictionary(const Dictionary::Ptr& dictionary);

public:
	MessagePart(void);
	MessagePart(string json);
	MessagePart(const Dictionary::Ptr& dictionary);
	MessagePart(const MessagePart& message);

	string ToJsonString(void) const;

	Dictionary::Ptr GetDictionary(void) const;

	/**
	 * Retrieves a property's value.
	 *
	 * @param key The name of the property.
	 * @param[out] The value.
	 * @returns true if the value was retrieved, false otherwise.
	 */
	template<typename T>
	bool GetProperty(string key, T *value) const
	{
		return GetDictionary()->GetProperty(key, value);
	}

	/**
	 * Sets a property's value.
	 *
	 * @param key The name of the property.
	 * @param value The value.
	 */
	template<typename T>
	void SetProperty(string key, const T& value)
	{
		GetDictionary()->SetProperty(key, value);
	}

	bool GetProperty(string key, MessagePart *value) const;
	void SetProperty(string key, const MessagePart& value);

	/**
	 * Adds an item to the message using an automatically generated property name.
	 *
	 * @param value The value.
	 */
	template<typename T>
	void AddUnnamedProperty(const T& value)
	{
		GetDictionary()->AddUnnamedProperty(value);
	}

	void AddUnnamedProperty(const MessagePart& value);

	DictionaryIterator Begin(void);
	DictionaryIterator End(void);
};

}

#endif /* MESSAGEPART_H */
