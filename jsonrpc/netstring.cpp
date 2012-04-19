#include <cstdio>
#include "i2-jsonrpc.h"
#include <cJSON.h>

using namespace icinga;

Dictionary::Ptr Netstring::GetDictionaryFromJson(json_t *json)
{
	Dictionary::Ptr dictionary = make_shared<Dictionary>();

	for (cJSON *i = json->child; i != NULL; i = i->next) {
		switch (i->type) {
			case cJSON_Number:
				dictionary->SetValueInteger(i->string, i->valueint);
				break;
			case cJSON_String:
				dictionary->SetValueString(i->string, i->valuestring);
				break;
			case cJSON_Object:
				dictionary->SetValueDictionary(i->string, GetDictionaryFromJson(i));
				break;
			default:
				break;
		}
	}

	return dictionary;
}

json_t *Netstring::GetJsonFromDictionary(const Dictionary::Ptr& dictionary)
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

				if (valueDictionary.get() != NULL)
					cJSON_AddItemToObject(json, i->first.c_str(), GetJsonFromDictionary(valueDictionary));
			default:
				break;
		}
	}

	return json;
}

/* based on https://github.com/PeterScott/netstring-c/blob/master/netstring.c */
bool Netstring::ReadMessageFromFIFO(FIFO::Ptr fifo, Message *message)
{
	size_t buffer_length = fifo->GetSize();
	char *buffer = (char *)fifo->GetReadBuffer();

	/* minimum netstring length is 3 */
	if (buffer_length < 3)
		return false;

	/* no leading zeros allowed */
	if (buffer[0] == '0' && isdigit(buffer[1]))
		throw InvalidArgumentException("Invalid netstring (leading zero)");

	size_t len, i;

	len = 0;
	for (i = 0; i < buffer_length && isdigit(buffer[i]); i++) {
		/* length specifier must have at most 9 characters */
		if (i >= 9)
			return false;

		len = len * 10 + (buffer[i] - '0');
	}

	/* make sure the buffer is large enough */
	if (i + len + 1 >= buffer_length)
		return false;

	/* check for the colon delimiter */
	if (buffer[i++] != ':')
		throw InvalidArgumentException("Invalid Netstring (missing :)");

	/* check for the comma delimiter after the string */
	if (buffer[i + len] != ',')
		throw InvalidArgumentException("Invalid Netstring (missing ,)");

	/* nuke the comma delimiter */
	buffer[i + len] = '\0';
	cJSON *object = cJSON_Parse(&buffer[i]);

	if (object == NULL) {
		/* restore the comma */
		buffer[i + len] = ',';
		throw InvalidArgumentException("Invalid JSON string");
	}

	/* remove the data from the fifo */
	fifo->Read(NULL, i + len + 1);

	*message = Message(GetDictionaryFromJson(object));
	cJSON_Delete(object);
	return true;
}

void Netstring::WriteMessageToFIFO(FIFO::Ptr fifo, const Message& message)
{
	char *json;
	cJSON *object = GetJsonFromDictionary(message.GetDictionary());
	size_t len;

#ifdef _DEBUG
	json = cJSON_Print(object);
#else /* _DEBUG */
	json = cJSON_PrintUnformatted(object);
#endif /* _DEBUG */

	cJSON_Delete(object);

	len = strlen(json);
	char strLength[50];
	sprintf(strLength, "%lu:", (unsigned long)len);

	fifo->Write(strLength, strlen(strLength));
	fifo->Write(json, len);
	free(json);

	fifo->Write(",", 1);
}
