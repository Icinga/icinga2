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
