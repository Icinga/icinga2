#include "i2-base.h"

#if I2_PLATFORM == PLATFORM_UNIX
#include <ltdl.h>

using namespace icinga;

void Sleep(unsigned long milliseconds)
{
	usleep(milliseconds * 1000);
}

inline void closesocket(SOCKET fd)
{
	close(fd);
}

inline HMODULE LoadLibrary(const char *filename)
{
	lt_dlhandle handle = 0;
	lt_dladvise advise;

	if (!lt_dladvise_init(&advise) && !lt_dladvise_global(&advise)) {
		handle = lt_dlopenadvise(filename, advise);
	}

	lt_dladvise_destroy(&advise);

	return handle;
}

inline void FreeLibrary(HMODULE module)
{
	if (module)
		lt_dlclose(module);
}

inline void *GetProcAddress(HMODULE module, const char *function)
{
	return lt_dlsym(module, function);
}

#endif /* I2_PLATFORM == PLATFORM_UNIX */
