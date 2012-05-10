/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

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
