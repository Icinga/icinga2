#ifndef UNIX_H
#define UNIX_H

#include <ltdl.h>
#include <execinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef int SOCKET;

#define INVALID_SOCKET (-1)

inline void Sleep(unsigned long milliseconds)
{
	usleep(milliseconds * 1000);
}

inline void closesocket(int fd)
{
	close(fd);
}

#define ioctlsocket ioctl

/* default visibility takes care of exported symbols */
#define I2_EXPORT
#define I2_IMPORT

typedef lt_dlhandle HMODULE;

#define INVALID_HANDLE_VALUE NULL

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

#endif /* UNIX_H */
