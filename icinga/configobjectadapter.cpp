#include "i2-icinga.h"

using namespace icinga;

string ConfigObjectAdapter::GetType(void) const
{
	return m_ConfigObject->GetType();
}

string ConfigObjectAdapter::GetName(void) const
{
	return m_ConfigObject->GetName();
}

bool ConfigObjectAdapter::IsLocal(void) const
{
	return m_ConfigObject->IsLocal();
}

ConfigObject::Ptr ConfigObjectAdapter::GetConfigObject() const
{
	return m_ConfigObject;
}
