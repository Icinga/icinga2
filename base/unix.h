#ifndef UNIX_H
#define UNIX_H

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

void Sleep(unsigned long milliseconds);

typedef int SOCKET;
#define INVALID_SOCKET (-1)
void closesocket(SOCKET fd);

#define ioctlsocket ioctl

/* default visibility takes care of exported symbols */
#define I2_EXPORT
#define I2_IMPORT

typedef void *HMODULE;

#define INVALID_HANDLE_VALUE NULL

HMODULE LoadLibrary(const char *filename);
void FreeLibrary(HMODULE module);
void *GetProcAddress(HMODULE module, const char *function);

#endif /* UNIX_H */
