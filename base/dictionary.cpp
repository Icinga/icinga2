#include "i2-base.h"

using namespace icinga;

bool Dictionary::GetValueVariant(string key, Variant *value)
{
	DictionaryIterator i = m_Data.find(key);

	if (i == m_Data.end())
		return false;

	*value = i->second;

	return true;
}

void Dictionary::SetValueVariant(string key, const Variant& value)
{
	m_Data.erase(key);
	m_Data[key] = value;
}

bool Dictionary::GetValueString(string key, string *value)
{
	Variant data;

	if (!GetValueVariant(key, &data))
		return false;

	*value = static_cast<string>(data);
	return true;
}

void Dictionary::SetValueString(string key, const string& value)
{
	SetValueVariant(key, Variant(value));
}

bool Dictionary::GetValueInteger(string key, long *value)
{
	Variant data;

	if (!GetValueVariant(key, &data))
		return false;

	*value = data;
	return true;
}

void Dictionary::SetValueInteger(string key, long value)
{
	SetValueVariant(key, Variant(value));
}

bool Dictionary::GetValueDictionary(string key, Dictionary::Ptr *value)
{
	Dictionary::Ptr dictionary;
	Variant data;

	if (!GetValueVariant(key, &data))
		return false;

	dictionary = dynamic_pointer_cast<Dictionary>(data.GetObject());

	if (dictionary == NULL)
		throw InvalidArgumentException();

	*value = dictionary;

	return true;
}

void Dictionary::SetValueDictionary(string key, const Dictionary::Ptr& value)
{
	SetValueVariant(key, Variant(value));
}

bool Dictionary::GetValueObject(string key, Object::Ptr *value)
{
	Variant data;

	if (!GetValueVariant(key, &data))
		return false;

	*value = data;
	return true;
}

void Dictionary::SetValueObject(string key, const Object::Ptr& value)
{
	SetValueVariant(key, Variant(value));
}

DictionaryIterator Dictionary::Begin(void)
{
	return m_Data.begin();
}

DictionaryIterator Dictionary::End(void)
{
	return m_Data.end();
}
