#include "i2-base.h"

using namespace icinga;

/**
 * SetApplication
 *
 * Sets the application this component belongs to.
 *
 * @param application The application.
 */
void Component::SetApplication(const Application::WeakPtr& application)
{
	m_Application = application;
}

/**
 * GetApplication
 *
 * Retrieves the application this component belongs to.
 *
 * @returns The application.
 */
Application::Ptr Component::GetApplication(void) const
{
	return m_Application.lock();
}

/**
 * SetConfig
 *
 * Sets the configuration for this component.
 *
 * @param componentConfig The configuration.
 */
void Component::SetConfig(const ConfigObject::Ptr& componentConfig)
{
	m_Config = componentConfig;
}

/**
 * GetConfig
 *
 * Retrieves the configuration for this component.
 *
 * @returns The configuration.
 */
ConfigObject::Ptr Component::GetConfig(void) const
{
	return m_Config;
}
