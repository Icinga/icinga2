#include "i2-base.h"

using namespace icinga;

Variant::Variant(void) : m_Type(VariantEmpty)
{	
}

Variant::Variant(long value) : m_Type(VariantInteger), m_IntegerValue(value)
{
}

Variant::Variant(string value) : m_Type(VariantString), m_StringValue(value)
{
}

Variant::Variant(Object::Ptr value) : m_Type(VariantObject), m_ObjectValue(value)
{
}

void Variant::Convert(VariantType newType) const
{
	if (newType == m_Type)
		return;

	throw NotImplementedException();
}

VariantType Variant::GetType(void) const
{
	return m_Type;
}

long Variant::GetInteger(void) const
{
	Convert(VariantInteger);

	return m_IntegerValue;
}

string Variant::GetString(void) const
{
	Convert(VariantString);

	return m_StringValue;
}

Object::Ptr Variant::GetObject(void) const
{
	Convert(VariantObject);

	return m_ObjectValue;
}

Variant::operator long(void) const
{
	return GetInteger();
}

Variant::operator string(void) const
{
	return GetString();
}

Variant::operator Object::Ptr(void) const
{
	return GetObject();
}
