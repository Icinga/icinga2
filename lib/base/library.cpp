/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "base/library.hpp"
#include "base/loader.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"

using namespace icinga;

/**
 * Loads the specified library.
 *
 * @param name The name of the library.
 */
Library::Library(const String& name)
{
	String path;
#if defined(_WIN32)
	path = name + ".dll";
#elif defined(__APPLE__)
	path = "lib" + name + "." + Application::GetAppSpecVersion() + ".dylib";
#else /* __APPLE__ */
	path = "lib" + name + ".so." + Application::GetAppSpecVersion();
#endif /* _WIN32 */

	Log(LogNotice, "Library")
		<< "Loading library '" << path << "'";

#ifdef _WIN32
	HMODULE hModule = LoadLibrary(path.CStr());

	if (!hModule) {
		BOOST_THROW_EXCEPTION(win32_error()
			<< boost::errinfo_api_function("LoadLibrary")
			<< errinfo_win32_error(GetLastError())
			<< boost::errinfo_file_name(path));
	}
#else /* _WIN32 */
	void *hModule = dlopen(path.CStr(), RTLD_NOW | RTLD_GLOBAL);

	if (!hModule) {
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not load library '" + path + "': " + dlerror()));
	}
#endif /* _WIN32 */

	Loader::ExecuteDeferredInitializers();

	m_Handle.reset(new LibraryHandle(hModule), [](LibraryHandle *handle) {
#ifdef _WIN32
		FreeLibrary(*handle);
#else /* _WIN32 */
		dlclose(*handle);
#endif /* _WIN32 */
	});
}

void *Library::GetSymbolAddress(const String& name) const
{
	if (!m_Handle)
		BOOST_THROW_EXCEPTION(std::runtime_error("Invalid library handle"));

#ifdef _WIN32
	return GetProcAddress(*m_Handle.get(), name.CStr());
#else /* _WIN32 */
	return dlsym(*m_Handle.get(), name.CStr());
#endif /* _WIN32 */
}
