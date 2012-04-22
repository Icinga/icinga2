#include "i2-base.h"

using namespace icinga;

bool Dictionary::GetProperty(string key, Variant *value) const
{
	ConstDictionaryIterator i = m_Data.find(key);

	if (i == m_Data.end())
		return false;

	*value = i->second;
	return true;
}

void Dictionary::SetProperty(string key, const Variant& value)
{
	DictionaryIterator i = m_Data.find(key);

	Variant oldValue;
	if (i != m_Data.end()) {
		oldValue = i->second;
		m_Data.erase(i);
	}

	m_Data[key] = value;

	PropertyChangedEventArgs dpce;
	dpce.Source = shared_from_this();
	dpce.Property = key;
	dpce.OldValue = oldValue;
	dpce.NewValue = value;
	OnPropertyChanged(dpce);
}

bool Dictionary::GetPropertyString(string key, string *value)
{
	Variant data;

	if (!GetProperty(key, &data))
		return false;

	*value = static_cast<string>(data);
	return true;
}

void Dictionary::SetPropertyString(string key, const string& value)
{
	SetProperty(key, Variant(value));
}

bool Dictionary::GetPropertyInteger(string key, long *value)
{
	Variant data;

	if (!GetProperty(key, &data))
		return false;

	*value = static_cast<long>(data);
	return true;
}

void Dictionary::SetPropertyInteger(string key, long value)
{
	SetProperty(key, Variant(value));
}

bool Dictionary::GetPropertyDictionary(string key, Dictionary::Ptr *value)
{
	Dictionary::Ptr dictionary;
	Variant data;

	if (!GetProperty(key, &data))
		return false;

	dictionary = dynamic_pointer_cast<Dictionary>(data.GetObject());

	if (dictionary == NULL)
		throw InvalidArgumentException();

	*value = dictionary;

	return true;
}

void Dictionary::SetPropertyDictionary(string key, const Dictionary::Ptr& value)
{
	SetProperty(key, Variant(value));
}

bool Dictionary::GetPropertyObject(string key, Object::Ptr *value)
{
	Variant data;

	if (!GetProperty(key, &data))
		return false;

	*value = data;
	return true;
}

void Dictionary::SetPropertyObject(string key, const Object::Ptr& value)
{
	SetProperty(key, Variant(value));
}

DictionaryIterator Dictionary::Begin(void)
{
	return m_Data.begin();
}

DictionaryIterator Dictionary::End(void)
{
	return m_Data.end();
}
