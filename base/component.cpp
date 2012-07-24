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

map<string, Component::Ptr> Component::m_Components;

/**
 * Loads a component from a shared library.
 *
 * @param name The name of the component.
 * @param componentConfig The configuration for the component.
 */
void Component::Load(const string& name, const ConfigObject::Ptr& config)
{
	assert(Application::IsMainThread());

	string path;
#ifdef _WIN32
	path = name + ".dll";
#else /* _WIN32 */
	path = name + ".la";
#endif /* _WIN32 */

	Logger::Write(LogInformation, "base", "Loading component '" + name + "' (using library '" + path + "')");

#ifdef _WIN32
	HMODULE hModule = LoadLibrary(path.c_str());

	if (hModule == NULL)
		throw_exception(Win32Exception("LoadLibrary('" + path + "') failed", GetLastError()));
#else /* _WIN32 */
	lt_dlhandle hModule = lt_dlopen(path.c_str());

	if (hModule == NULL) {
		throw_exception(runtime_error("Could not load module '" + path + "': " +  lt_dlerror()));
	}
#endif /* _WIN32 */

	CreateComponentFunction pCreateComponent;

#ifdef _WIN32
	pCreateComponent = (CreateComponentFunction)GetProcAddress(hModule,
	    "CreateComponent");
#else /* _WIN32 */
#	ifdef __GNUC__
	/* suppress compiler warning for void * cast */
	__extension__
#	endif
	pCreateComponent = (CreateComponentFunction)lt_dlsym(hModule,
	    "CreateComponent");
#endif /* _WIN32 */

	Component::Ptr component;

	try {
		if (pCreateComponent == NULL)
			throw_exception(runtime_error("Loadable module does not contain "
			    "CreateComponent function"));

		component = Component::Ptr(pCreateComponent());

		if (!component)
			throw_exception(runtime_error("CreateComponent function returned NULL."));
	} catch (...) {
#ifdef _WIN32
		FreeLibrary(hModule);
#else /* _WIN32 */
		lt_dlclose(hModule);
#endif /* _WIN32 */
		throw;
	}

	component->m_Name = name;
	component->m_Config = config;

	try {
		m_Components[name] = component;
		component->Start();
	} catch (...) {
		m_Components.erase(name);
		throw;
	}
}

void Component::Unload(const string& componentName)
{
	map<string, Component::Ptr>::iterator it;
	
	it = m_Components.find(componentName);

	if (it == m_Components.end())
		return;

	Logger::Write(LogInformation, "base", "Unloading component '" + componentName + "'");

	Component::Ptr component = it->second;
	component->Stop();

	m_Components.erase(it);

	/** Unfortunatelly we can't safely unload the DLL/shared library
	 * here because there could still be objects that use the library. */
}

void Component::UnloadAll(void)
{
	Logger::Write(LogInformation, "base", "Unloading all components");

	while (!m_Components.empty()) {
		string name = m_Components.begin()->first;
		Unload(name);
	}
}

/**
 * Adds a directory to the component search path.
 *
 * @param componentDirectory The directory.
 */
void Component::AddSearchDir(const string& componentDirectory)
{
#ifdef _WIN32
	SetDllDirectory(componentDirectory.c_str());
#else /* _WIN32 */
	lt_dladdsearchdir(componentDirectory.c_str());
#endif /* _WIN32 */
}

/**
 * Retrieves the name of the component.
 *
 * @returns Name of the component.
 */
string Component::GetName(void) const
{
	return m_Name;
}

/**
 * Retrieves the configuration for this component.
 *
 * @returns The configuration.
 */
ConfigObject::Ptr Component::GetConfig(void) const
{
	return m_Config;
}

/**
 * Starts the component.
 */
void Component::Start(void)
{
	/* Nothing to do in the default implementation. */
}

/**
 * Stops the component.
 */
void Component::Stop(void)
{
	/* Nothing to do in the default implementation. */
}
