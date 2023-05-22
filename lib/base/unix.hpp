/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include <limits.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <netdb.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libgen.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <glob.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <strings.h>
#include <errno.h>

typedef int SOCKET;
#define INVALID_SOCKET (-1)

#define closesocket close
#define ioctlsocket ioctl

#ifndef SUN_LEN
/* TODO: Ideally this should take into the account how
 * long the socket path really is.
 */
#	define SUN_LEN(sun) (sizeof(sockaddr_un))
#endif /* SUN_LEN */

#ifndef PATH_MAX
#	define PATH_MAX 1024
#endif /* PATH_MAX */

#ifndef MAXPATHLEN
#	define MAXPATHLEN PATH_MAX
#endif /* MAXPATHLEN */
