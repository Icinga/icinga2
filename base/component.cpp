#include "i2-base.h"

using namespace icinga;

void Component::SetApplication(const Application::WeakPtr& application)
{
	m_Application = application;
}

Application::Ptr Component::GetApplication(void)
{
	return m_Application.lock();
}

void Component::SetConfig(ConfigObject::Ptr componentConfig)
{
	m_Config = componentConfig;
}

ConfigObject::Ptr Component::GetConfig(void)
{
	return m_Config;
}
