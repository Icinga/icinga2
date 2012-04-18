#ifndef DICTIONARY_H
#define DICTIONARY_H

namespace icinga
{

typedef map<string, Variant>::iterator DictionaryIterator;

class I2_BASE_API Dictionary : public Object
{
private:
	map<string, Variant> m_Data;

public:
	typedef shared_ptr<Dictionary> Ptr;
	typedef weak_ptr<Dictionary> WeakPtr;

	bool GetValueVariant(string key, Variant *value);
	void SetValueVariant(string key, const Variant& value);

	bool GetValueString(string key, string *value);
	void SetValueString(string key, const string& value);

	bool GetValueInteger(string key, long *value);
	void SetValueInteger(string key, long value);

	bool GetValueDictionary(string key, Dictionary::Ptr *value);
	void SetValueDictionary(string key, const Dictionary::Ptr& value);

	bool GetValueObject(string key, Object::Ptr *value);
	void SetValueObject(string key, const Object::Ptr& value);

	DictionaryIterator Begin(void);
	DictionaryIterator End(void);
};

}

#endif /* DICTIONARY_H */
