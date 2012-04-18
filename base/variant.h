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
	Variant(void);
	Variant(long value);
	Variant(string value);
	Variant(Object::Ptr value);

	VariantType GetType(void) const;

	long GetInteger(void) const;
	string GetString(void) const;
	Object::Ptr GetObject(void) const;

	operator long(void) const;
	operator string(void) const;
	operator Object::Ptr(void) const;
};

}

#endif /* VARIANT_H */