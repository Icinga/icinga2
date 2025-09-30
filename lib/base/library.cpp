/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
	path = "lib" + name + "." + Application::GetAppPackageVersion() + ".dylib";
#else /* __APPLE__ */
	path = "lib" + name + ".so." + Application::GetAppPackageVersion();
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
