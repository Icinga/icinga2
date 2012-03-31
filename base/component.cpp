#include "i2-base.h"

using namespace icinga;

void Component::SetApplication(const Application::WeakRefType& application)
{
	m_Application = application;
}

Application::RefType Component::GetApplication(void)
{
	return m_Application.lock();
}

void Component::SetConfig(ConfigObject::RefType componentConfig)
{
	m_Config = componentConfig;
}

ConfigObject::RefType Component::GetConfig(void)
{
	return m_Config;
}
