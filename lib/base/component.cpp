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
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-base.h"

using namespace icinga;

REGISTER_TYPE(Component);

boost::mutex Component::m_Mutex;
map<String, Component::Factory> Component::m_Factories;

/**
 * Constructor for the component class.
 */
Component::Component(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	if (!IsLocal())
		BOOST_THROW_EXCEPTION(runtime_error("Component objects must be local."));

	Logger::Write(LogInformation, "base", "Loading component '" + GetName() + "'");

	(void) Utility::LoadIcingaLibrary(GetName(), true);

	Component::Factory factory;

	{
		boost::mutex::scoped_lock lock(m_Mutex);

		map<String, Factory>::iterator it;
		it = m_Factories.find(GetName());

		if (it == m_Factories.end())
			BOOST_THROW_EXCEPTION(invalid_argument("Unknown component: " + GetName()));

		factory = it->second;
	}

	m_Impl = factory();

	if (!m_Impl)
		BOOST_THROW_EXCEPTION(runtime_error("Component factory returned NULL."));
}

/**
 * Destructor for the Component class.
 */
Component::~Component(void)
{
	if (m_Impl)
		m_Impl->Stop();
}

/**
 * Starts the component. Called when the DynamicObject is fully
 * constructed/registered.
 */
void Component::Start(void)
{
	ObjectLock olock(this);

	m_Impl->SetConfig(GetSelf());
	m_Impl->Start();
}

/**
 * Adds a directory to the component search path.
 *
 * @param componentDirectory The directory.
 */
void Component::AddSearchDir(const String& componentDirectory)
{
	Logger::Write(LogInformation, "base", "Adding library search dir: " +
	    componentDirectory);

#ifdef _WIN32
	SetDllDirectory(componentDirectory.CStr());
#else /* _WIN32 */
	lt_dladdsearchdir(componentDirectory.CStr());
#endif /* _WIN32 */
}

/**
 * Retrieves the configuration for this component.
 *
 * @returns The configuration.
 */
DynamicObject::Ptr IComponent::GetConfig(void) const
{
	ObjectLock olock(this);

	return m_Config.lock();
}

/**
 * Sets the configuration for this component.
 */
void IComponent::SetConfig(const DynamicObject::Ptr& config)
{
	ObjectLock olock(this);

	m_Config = config;
}

/**
 * Starts the component.
 */
void IComponent::Start(void)
{
	/* Nothing to do in the default implementation. */
}

/**
 * Stops the component.
 */
void IComponent::Stop(void)
{
	/* Nothing to do in the default implementation. */
}

/**
 * Registers a component factory.
 */
void Component::Register(const String& name, const Component::Factory& factory)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	m_Factories[name] = factory;
}
