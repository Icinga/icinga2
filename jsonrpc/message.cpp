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

#include "i2-jsonrpc.h"

using namespace icinga;

Message::Message(void)
{
	m_Dictionary = make_shared<Dictionary>();
}

Message::Message(const Dictionary::Ptr& dictionary)
{
	m_Dictionary = dictionary;
}

Message::Message(const Message& message)
{
	m_Dictionary = message.GetDictionary();
}

Dictionary::Ptr Message::GetDictionary(void) const
{
	return m_Dictionary;
}

bool Message::GetPropertyString(string key, string *value) const
{
	return GetDictionary()->GetPropertyString(key, value);
}

bool Message::GetPropertyInteger(string key, long *value) const
{
	return GetDictionary()->GetPropertyInteger(key, value);
}

bool Message::GetPropertyMessage(string key, Message *value) const
{
	Dictionary::Ptr dictionary;
	if (!GetDictionary()->GetPropertyDictionary(key, &dictionary))
		return false;

	*value = Message(dictionary);
	return true;
}

void Message::SetPropertyString(string key, const string& value)
{
	GetDictionary()->SetProperty(key, value);
}

void Message::SetPropertyInteger(string key, long value)
{
	GetDictionary()->SetProperty(key, value);
}

void Message::SetPropertyMessage(string key, const Message& value)
{
	GetDictionary()->SetProperty(key, Variant(value.GetDictionary()));
}

void Message::AddUnnamedPropertyString(const string& value)
{
	GetDictionary()->AddUnnamedPropertyString(value);
}

void Message::AddUnnamedPropertyInteger(long value)
{
	GetDictionary()->AddUnnamedPropertyInteger(value);
}

void Message::AddUnnamedPropertyMessage(const Message& value)
{
	GetDictionary()->AddUnnamedPropertyDictionary(value.GetDictionary());
}
