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

REGISTER_TYPE(Component, NULL);

/**
 * Constructor for the component class.
 */
Component::Component(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	assert(Application::IsMainThread());

	if (!IsLocal())
		BOOST_THROW_EXCEPTION(runtime_error("Component objects must be local."));

#ifdef _WIN32
	HMODULE
#else /* _WIN32 */
	lt_dlhandle
#endif /* _WIN32 */
	hModule;

	Logger::Write(LogInformation, "base", "Loading component '" + GetName() + "'");

	hModule = Utility::LoadIcingaLibrary(GetName(), true);

	CreateComponentFunction pCreateComponent;

#ifdef _WIN32
	pCreateComponent = reinterpret_cast<CreateComponentFunction>(GetProcAddress(hModule,
	    "CreateComponent"));
#else /* _WIN32 */
#	ifdef __GNUC__
	/* suppress compiler warning for void * cast */
	__extension__
#	endif
	pCreateComponent = reinterpret_cast<CreateComponentFunction>(lt_dlsym(hModule,
	    "CreateComponent"));
#endif /* _WIN32 */

	IComponent::Ptr impl;

	try {
		if (pCreateComponent == NULL)
			BOOST_THROW_EXCEPTION(runtime_error("Loadable module does not contain "
			    "CreateComponent function"));

		/* pCreateComponent returns a raw pointer which we must wrap in a shared_ptr */
		impl = IComponent::Ptr(pCreateComponent());

		if (!impl)
			BOOST_THROW_EXCEPTION(runtime_error("CreateComponent function returned NULL."));
	} catch (...) {
#ifdef _WIN32
		FreeLibrary(hModule);
#else /* _WIN32 */
		lt_dlclose(hModule);
#endif /* _WIN32 */
		throw;
	}

	m_Impl = impl;
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
	m_Impl->m_Config = GetSelf();
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
	return m_Config.lock();
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
