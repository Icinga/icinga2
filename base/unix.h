#ifndef I2_UNIX_H
#define I2_UNIX_H

#include <execinfo.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

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

#endif /* I2_UNIX_H */