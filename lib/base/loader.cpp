/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/loader.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/application.hpp"

using namespace icinga;

/**
 * Loads the specified library.
 *
 * @param library The name of the library.
 */
void Loader::LoadExtensionLibrary(const String& library)
{
	String path;
#if defined(_WIN32)
	path = library + ".dll";
#elif defined(__APPLE__)
	path = "lib" + library + "." + Application::GetAppSpecVersion() + ".dylib";
#else /* __APPLE__ */
	path = "lib" + library + ".so." + Application::GetAppSpecVersion();
#endif /* _WIN32 */

	Log(LogNotice, "Loader")
	    << "Loading library '" << path << "'";

#ifdef _WIN32
	HMODULE hModule = LoadLibrary(path.CStr());

	if (hModule == NULL) {
		BOOST_THROW_EXCEPTION(win32_error()
		    << boost::errinfo_api_function("LoadLibrary")
		    << errinfo_win32_error(GetLastError())
		    << boost::errinfo_file_name(path));
	}
#else /* _WIN32 */
	void *hModule = dlopen(path.CStr(), RTLD_NOW | RTLD_GLOBAL);

	if (hModule == NULL) {
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not load library '" + path + "': " + dlerror()));
	}
#endif /* _WIN32 */

	ExecuteDeferredInitializers();
}

boost::thread_specific_ptr<std::priority_queue<DeferredInitializer> >& Loader::GetDeferredInitializers(void)
{
	static boost::thread_specific_ptr<std::priority_queue<DeferredInitializer> > initializers;
	return initializers;
}

void Loader::ExecuteDeferredInitializers(void)
{
	if (!GetDeferredInitializers().get())
		return;

	while (!GetDeferredInitializers().get()->empty()) {
		DeferredInitializer initializer = GetDeferredInitializers().get()->top();
		GetDeferredInitializers().get()->pop();
		initializer();
	}
}

void Loader::AddDeferredInitializer(const std::function<void(void)>& callback, int priority)
{
	if (!GetDeferredInitializers().get())
		GetDeferredInitializers().reset(new std::priority_queue<DeferredInitializer>());

	GetDeferredInitializers().get()->push(DeferredInitializer(callback, priority));
}

