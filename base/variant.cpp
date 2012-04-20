#include "i2-base.h"

using namespace icinga;

void Variant::Convert(VariantType newType) const
{
	if (newType == m_Type)
		return;

	if (m_Type == VariantString && newType == VariantInteger) {
		m_IntegerValue = strtol(m_StringValue.c_str(), NULL, 10);
		m_Type = VariantInteger;

		return;
	}

	// TODO: convert variant data
	throw InvalidArgumentException("Invalid variant conversion.");
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

bool Variant::IsEmpty(void) const
{
	return (m_Type == VariantEmpty);
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
