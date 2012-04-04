#ifndef UNIX_H
#define UNIX_H

#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>

void Sleep(unsigned long milliseconds);

typedef int SOCKET;
#define INVALID_SOCKET (-1)
void closesocket(SOCKET fd);

#define ioctlsocket ioctl

#define MAXPATHLEN PATH_MAX

/* default visibility takes care of exported symbols */
#define I2_EXPORT
#define I2_IMPORT

#endif /* UNIX_H */
