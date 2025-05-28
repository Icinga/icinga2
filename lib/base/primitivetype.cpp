/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/primitivetype.hpp"
#include "base/dictionary.hpp"

using namespace icinga;

PrimitiveType::PrimitiveType(String name, String base, const ObjectFactory& factory)
	: m_Name(std::move(name)), m_Base(std::move(base)), m_Factory(factory)
{ }

String PrimitiveType::GetName() const
{
	return m_Name;
}

Type::Ptr PrimitiveType::GetBaseType() const
{
	if (m_Base == "None")
		return nullptr;
	else
		return Type::GetByName(m_Base);
}

int PrimitiveType::GetAttributes() const
{
	return 0;
}

int PrimitiveType::GetFieldId(const String& name) const
{
	Type::Ptr base = GetBaseType();

	if (base)
		return base->GetFieldId(name);
	else
		return -1;
}

Field PrimitiveType::GetFieldInfo(int id) const
{
	Type::Ptr base = GetBaseType();

	if (base)
		return base->GetFieldInfo(id);
	else
		throw std::runtime_error("Invalid field ID.");
}

int PrimitiveType::GetFieldCount() const
{
	Type::Ptr base = GetBaseType();

	if (base)
		return Object::TypeInstance->GetFieldCount();
	else
		return 0;
}

ObjectFactory PrimitiveType::GetFactory() const
{
	return m_Factory;
}
