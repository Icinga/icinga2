#include "i2-base.h"

using namespace icinga;

void Component::SetApplication(const Application::WeakPtr& application)
{
	m_Application = application;
}

Application::Ptr Component::GetApplication(void) const
{
	return m_Application.lock();
}

void Component::SetConfig(const ConfigObject::Ptr& componentConfig)
{
	m_Config = componentConfig;
}

ConfigObject::Ptr Component::GetConfig(void) const
{
	return m_Config;
}
