/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "config/objectrule.hpp"
#include <set>

using namespace icinga;

ObjectRule::TypeSet ObjectRule::m_Types;

void ObjectRule::RegisterType(const String& sourceType)
{
	m_Types.insert(sourceType);
}

bool ObjectRule::IsValidSourceType(const String& sourceType)
{
	return m_Types.find(sourceType) != m_Types.end();
}
