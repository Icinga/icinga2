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

#include "i2-jsonrpc.h"
#include <cJSON.h>

using namespace icinga;

MessagePart::MessagePart(void)
{
	m_Dictionary = make_shared<Dictionary>();
}

MessagePart::MessagePart(string jsonString)
{
	json_t *json = cJSON_Parse(jsonString.c_str());

	if (!json)
		throw InvalidArgumentException("Invalid JSON string");

	m_Dictionary = GetDictionaryFromJson(json);

	cJSON_Delete(json);
}

MessagePart::MessagePart(const Dictionary::Ptr& dictionary)
{
	m_Dictionary = dictionary;
}

MessagePart::MessagePart(const MessagePart& message)
{
	m_Dictionary = message.GetDictionary();
}

Dictionary::Ptr MessagePart::GetDictionaryFromJson(json_t *json)
{
	Dictionary::Ptr dictionary = make_shared<Dictionary>();

	for (cJSON *i = json->child; i != NULL; i = i->next) {
		switch (i->type) {
			case cJSON_Number:
				dictionary->SetProperty(i->string, i->valueint);
				break;
			case cJSON_String:
				dictionary->SetProperty(i->string, i->valuestring);
				break;
			case cJSON_Object:
				dictionary->SetProperty(i->string, GetDictionaryFromJson(i));
				break;
			default:
				break;
		}
	}

	return dictionary;
}

json_t *MessagePart::GetJsonFromDictionary(const Dictionary::Ptr& dictionary)
{
	cJSON *json;
	string valueString;
	Dictionary::Ptr valueDictionary;

	json = cJSON_CreateObject();

	for (DictionaryIterator i = dictionary->Begin(); i != dictionary->End(); i++) {
		switch (i->second.GetType()) {
			case VariantInteger:
				cJSON_AddNumberToObject(json, i->first.c_str(), i->second.GetInteger());
				break;
			case VariantString:
				valueString = i->second.GetString();
				cJSON_AddStringToObject(json, i->first.c_str(), valueString.c_str());
				break;
			case VariantObject:
				valueDictionary = dynamic_pointer_cast<Dictionary>(i->second.GetObject());

				if (valueDictionary)
					cJSON_AddItemToObject(json, i->first.c_str(), GetJsonFromDictionary(valueDictionary));
			default:
				break;
		}
	}

	return json;
}

string MessagePart::ToJsonString(void) const
{
	json_t *json = GetJsonFromDictionary(m_Dictionary);
	char *jsonString;
	string result;

#ifdef _DEBUG
	jsonString = cJSON_Print(json);
#else /* _DEBUG */
	jsonString = cJSON_PrintUnformatted(json);
#endif /* _DEBUG */

	cJSON_Delete(json);

	result = jsonString;

	free(jsonString);

	return result;
}

Dictionary::Ptr MessagePart::GetDictionary(void) const
{
	return m_Dictionary;
}

bool MessagePart::GetProperty(string key, MessagePart *value) const
{
	Object::Ptr object;
	if (GetDictionary()->GetProperty(key, &object))
		return false;

	Dictionary::Ptr dictionary = dynamic_pointer_cast<Dictionary>(object);
	if (!dictionary)
		throw InvalidCastException();

	*value = MessagePart(dictionary);
	return true;
}

void MessagePart::SetProperty(string key, const MessagePart& value)
{
	GetDictionary()->SetProperty(key, value.GetDictionary());
}

void MessagePart::AddUnnamedProperty(const MessagePart& value)
{
	GetDictionary()->AddUnnamedProperty(value.GetDictionary());
}

DictionaryIterator MessagePart::Begin(void)
{
	return GetDictionary()->Begin();
}

DictionaryIterator MessagePart::End(void)
{
	return GetDictionary()->End();
}

MessagePart::operator Dictionary::Ptr(void)
{
	return GetDictionary();
}
