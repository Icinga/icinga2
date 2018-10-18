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

#ifndef UNIX_H
#define UNIX_H

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
#endif /* UNIX_H */
