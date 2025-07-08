/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/reference.hpp"
#include "base/debug.hpp"
#include "base/primitivetype.hpp"
#include "base/dictionary.hpp"
#include "base/configwriter.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_PRIMITIVE_TYPE_NOINST(Reference, Object, Reference::GetPrototype());

Reference::Reference(const Object::Ptr& parent, const String& index)
	: m_Parent(parent), m_Index(index)
{
}

Value Reference::Get() const
{
	return m_Parent->GetFieldByName(m_Index, true, DebugInfo());
}

void Reference::Set(const Value& value)
{
	m_Parent->SetFieldByName(m_Index, value, DebugInfo());
}

Object::Ptr Reference::GetParent() const
{
	return m_Parent;
}

String Reference::GetIndex() const
{
	return m_Index;
}
