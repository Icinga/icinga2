#ifndef VARIANT_H
#define VARIANT_H

namespace icinga
{

enum I2_BASE_API VariantType
{
	VariantEmpty,
	VariantInteger,
	VariantString,
	VariantObject
};

class I2_BASE_API Variant
{
private:
	mutable long m_IntegerValue;
	mutable string m_StringValue;
	mutable Object::Ptr m_ObjectValue;

	mutable VariantType m_Type;

	void Convert(VariantType newType) const;

public:
	inline Variant(void) : m_Type(VariantEmpty) { }

	inline Variant(long value) : m_Type(VariantInteger), m_IntegerValue(value) { }

	inline Variant(const char *value) : m_Type(VariantString), m_StringValue(string(value)) { }

	inline Variant(string value) : m_Type(VariantString), m_StringValue(value) { }

	template<typename T>
	Variant(const shared_ptr<T>& value) : m_Type(VariantObject), m_ObjectValue(value) { }

	VariantType GetType(void) const;

	long GetInteger(void) const;
	string GetString(void) const;
	Object::Ptr GetObject(void) const;

	bool IsEmpty(void) const;

	operator long(void) const;
	operator string(void) const;
	operator Object::Ptr(void) const;
};

}

#endif /* VARIANT_H */