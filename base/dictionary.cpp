#include "i2-base.h"

using namespace icinga;

/**
 * GetProperty
 *
 * Retrieves a value from the dictionary.
 *
 * @param key The key.
 * @param value Pointer to the value.
 * @returns true if the value was retrieved, false otherwise.
 */
bool Dictionary::GetProperty(string key, Variant *value) const
{
	ConstDictionaryIterator i = m_Data.find(key);

	if (i == m_Data.end())
		return false;

	*value = i->second;
	return true;
}

/**
 * SetProperty
 *
 * Sets a value in the dictionary.
 *
 * @param key The key.
 * @param value The value.
 */
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

/**
 * GetPropertyString
 *
 * Retrieves a value from the dictionary and converts it to a string.
 *
 * @param key The key.
 * @param value Pointer to the value.
 * @returns true if the value was retrieved, false otherwise.
 */
bool Dictionary::GetPropertyString(string key, string *value)
{
	Variant data;

	if (!GetProperty(key, &data))
		return false;

	*value = static_cast<string>(data);
	return true;
}

/**
 * SetPropertyString
 *
 * Sets a value in the dictionary.
 *
 * @param key The key.
 * @param value The value.
 */
void Dictionary::SetPropertyString(string key, const string& value)
{
	SetProperty(key, Variant(value));
}

/**
 * GetPropertyInteger
 *
 * Retrieves a value from the dictionary and converts it to an integer.
 *
 * @param key The key.
 * @param value Pointer to the value.
 * @returns true if the value was retrieved, false otherwise.
 */
bool Dictionary::GetPropertyInteger(string key, long *value)
{
	Variant data;

	if (!GetProperty(key, &data))
		return false;

	*value = static_cast<long>(data);
	return true;
}

/**
 * SetPropertyInteger
 *
 * Sets a value in the dictionary.
 *
 * @param key The key.
 * @param value The value.
 */
void Dictionary::SetPropertyInteger(string key, long value)
{
	SetProperty(key, Variant(value));
}

/**
 * GetPropertyDictionary
 *
 * Retrieves a value from the dictionary and converts it to a dictionary.
 *
 * @param key The key.
 * @param value Pointer to the value.
 * @returns true if the value was retrieved, false otherwise.
 */
bool Dictionary::GetPropertyDictionary(string key, Dictionary::Ptr *value)
{
	Dictionary::Ptr dictionary;
	Variant data;

	if (!GetProperty(key, &data))
		return false;

	dictionary = dynamic_pointer_cast<Dictionary>(data.GetObject());

	if (dictionary == NULL)
		throw InvalidArgumentException("Property is not a dictionary.");

	*value = dictionary;

	return true;
}

/**
 * SetPropertyDictionary
 *
 * Sets a value in the dictionary.
 *
 * @param key The key.
 * @param value The value.
 */
void Dictionary::SetPropertyDictionary(string key, const Dictionary::Ptr& value)
{
	SetProperty(key, Variant(value));
}

/**
 * GetPropertyObject
 *
 * Retrieves a value from the dictionary and converts it to an object.
 *
 * @param key The key.
 * @param value Pointer to the value.
 * @returns true if the value was retrieved, false otherwise.
 */
bool Dictionary::GetPropertyObject(string key, Object::Ptr *value)
{
	Variant data;

	if (!GetProperty(key, &data))
		return false;

	*value = data;
	return true;
}

/**
 * SetPropertyObject
 *
 * Sets a value in the dictionary.
 *
 * @param key The key.
 * @param value The value.
 */
void Dictionary::SetPropertyObject(string key, const Object::Ptr& value)
{
	SetProperty(key, Variant(value));
}

/**
 * Begin
 *
 * Returns an iterator to the beginning of the dictionary.
 *
 * @returns An iterator.
 */
DictionaryIterator Dictionary::Begin(void)
{
	return m_Data.begin();
}

/**
 * End
 *
 * Returns an iterator to the end of the dictionary.
 *
 * @returns An iterator.
 */
DictionaryIterator Dictionary::End(void)
{
	return m_Data.end();
}

/**
 * GetLength
 *
 * Returns the number of elements in the dictionary.
 *
 * @returns Number of elements.
 */
long Dictionary::GetLength(void) const
{
	return m_Data.size();
}

/**
 * AddUnnamedProperty
 *
 * Adds an unnamed value to the dictionary.
 *
 * @param value The value.
 */
void Dictionary::AddUnnamedProperty(const Variant& value)
{
	map<string, Variant>::const_iterator it;
	string key;
	do {
		long index = GetLength();
	
		stringstream s;
		s << "_" << GetLength();

		key = s.str();
		it = m_Data.find(key);
	} while (it != m_Data.end());

	m_Data[key] = value;
}

/**
 * AddUnnamedPropertyString
 *
 * Adds an unnamed value to the dictionary.
 *
 * @param value The value.
 */
void Dictionary::AddUnnamedPropertyString(const string& value)
{
	AddUnnamedProperty(Variant(value));
}

/**
 * AddUnnamedPropertyInteger
 *
 * Adds an unnamed value to the dictionary.
 *
 * @param value The value.
 */
void Dictionary::AddUnnamedPropertyInteger(long value)
{
	AddUnnamedProperty(Variant(value));
}

/**
 * AddUnnamedPropertyDictionary
 *
 * Adds an unnamed value to the dictionary.
 *
 * @param value The value.
 */
void Dictionary::AddUnnamedPropertyDictionary(const Dictionary::Ptr& value)
{
	AddUnnamedProperty(Variant(value));
}

/**
 * AddUnnamedPropertyObject
 *
 * Adds an unnamed value to the dictionary.
 *
 * @param value The value.
 */
void Dictionary::AddUnnamedPropertyObject(const Object::Ptr& value)
{
	AddUnnamedProperty(Variant(value));
}
