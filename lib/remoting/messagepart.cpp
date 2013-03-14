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

#include "i2-remoting.h"

using namespace icinga;

/**
 * Constructor for the MessagePart class.
 */
MessagePart::MessagePart(void)
{
	m_Dictionary = boost::make_shared<Dictionary>();
}

/**
 * Constructor for the MessagePart class.
 *
 * @param dictionary The dictionary that this MessagePart object should wrap.
 */
MessagePart::MessagePart(const Dictionary::Ptr& dictionary)
{
	m_Dictionary = dictionary;
}

/**
 * Copy-constructor for the MessagePart class.
 *
 * @param message The message that should be copied.
 */
MessagePart::MessagePart(const MessagePart& message)
{
	m_Dictionary = message.GetDictionary();
}

/**
 * Retrieves the underlying dictionary for this message.
 *
 * @returns A dictionary.
 */
Dictionary::Ptr MessagePart::GetDictionary(void) const
{
	return m_Dictionary;
}

/**
 * Retrieves a property's value.
 *
 * @param key The name of the property.
 * @param[out] value The value.
 * @returns true if the value was retrieved, false otherwise.
 */
bool MessagePart::Get(String key, MessagePart *value) const
{
	Value v;
	v = GetDictionary()->Get(key);

	if (!v.IsObjectType<Dictionary>())
		return false;

	Dictionary::Ptr dictionary = v;

	*value = MessagePart(dictionary);
	return true;
}

/**
 * Sets a property's value.
 *
 * @param key The name of the property.
 * @param value The value.
 */
void MessagePart::Set(String key, const MessagePart& value)
{
	GetDictionary()->Set(key, value.GetDictionary());
}

/**
 * Returns an iterator that points to the first element of the dictionary
 * which holds the properties for the message.
 *
 * @returns An iterator.
 */
Dictionary::Iterator MessagePart::Begin(void)
{
	return GetDictionary()->Begin();
}

/**
 * Returns an iterator that points past the last element of the dictionary
 * which holds the properties for the message.
 *
 * @returns An iterator.
 */
Dictionary::Iterator MessagePart::End(void)
{
	return GetDictionary()->End();
}

/**
 * Checks whether the message contains the specified element.
 *
 * @param key The name of the element.
 * @returns true if the message contains the element, false otherwise.
 */
bool MessagePart::Contains(const String& key) const
{
	return GetDictionary()->Contains(key);
}
