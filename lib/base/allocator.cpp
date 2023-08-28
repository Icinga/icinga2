/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#ifndef _WIN32

#include <cstddef>
#include <dlfcn.h>

extern "C" void* malloc(size_t bytes)
{
	static const auto libcMalloc = (void*(*)(size_t))dlsym(RTLD_NEXT, "malloc");
	return libcMalloc(bytes);
}

extern "C" void free(void* memory)
{
	static const auto libcFree = (void(*)(void*))dlsym(RTLD_NEXT, "free");
	libcFree(memory);
}

#endif /* _WIN32 */
