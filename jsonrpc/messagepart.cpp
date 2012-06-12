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

/**
 * Constructor for the MessagePart class.
 */
MessagePart::MessagePart(void)
{
	m_Dictionary = make_shared<Dictionary>();
}

/**
 * Constructor for the MessagePart class.
 *
 * @param jsonString The JSON string that should be used to initialize
 *		     the message.
 */
MessagePart::MessagePart(string jsonString)
{
	json_t *json = cJSON_Parse(jsonString.c_str());

	if (!json)
		throw runtime_error("Invalid JSON string");

	m_Dictionary = GetDictionaryFromJson(json);

	cJSON_Delete(json);
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
 * Converts a JSON object to a dictionary.
 *
 * @param json The JSON object.
 * @returns A dictionary that is equivalent to the JSON object.
 */
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

/**
 * Converts a dictionary to a JSON object.
 *
 * @param dictionary The dictionary.
 * @returns A JSON object that is equivalent to the dictionary. Values that
 *	    cannot be represented in JSON are omitted.
 */
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

/**
 * Converts a message into a JSON string.
 *
 * @returns A JSON string representing the message.
 */
string MessagePart::ToJsonString(void) const
{
	json_t *json = GetJsonFromDictionary(m_Dictionary);
	char *jsonString;
	string result;

	if (!Application::GetInstance()->IsDebugging())
		jsonString = cJSON_Print(json);
	else
		jsonString = cJSON_PrintUnformatted(json);

	cJSON_Delete(json);

	result = jsonString;

	free(jsonString);

	return result;
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
 * @param[out] The value.
 * @returns true if the value was retrieved, false otherwise.
 */
bool MessagePart::GetProperty(string key, MessagePart *value) const
{
	Object::Ptr object;
	if (!GetDictionary()->GetProperty(key, &object))
		return false;

	Dictionary::Ptr dictionary = dynamic_pointer_cast<Dictionary>(object);
	if (!dictionary)
		throw runtime_error("Object is not a dictionary.");

	*value = MessagePart(dictionary);
	return true;
}

/**
 * Sets a property's value.
 *
 * @param key The name of the property.
 * @param value The value.
 */
void MessagePart::SetProperty(string key, const MessagePart& value)
{
	GetDictionary()->SetProperty(key, value.GetDictionary());
}

/**
 * Adds an item to the message using an automatically generated property name.
 *
 * @param value The value.
 */
void MessagePart::AddUnnamedProperty(const MessagePart& value)
{
	GetDictionary()->AddUnnamedProperty(value.GetDictionary());
}

/**
 * Returns an iterator that points to the first element of the dictionary
 * which holds the properties for the message.
 *
 * @returns An iterator.
 */
DictionaryIterator MessagePart::Begin(void)
{
	return GetDictionary()->Begin();
}

/**
 * Returns an iterator that points past the last element of the dictionary
 * which holds the properties for the message.
 *
 * @returns An iterator.
 */
DictionaryIterator MessagePart::End(void)
{
	return GetDictionary()->End();
}
