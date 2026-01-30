// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WIN32_H
#define WIN32_H

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#endif /* _WIN32_WINNT */
#define NOMINMAX
#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <imagehlp.h>
#include <shlwapi.h>

#include <direct.h>

#ifdef __MINGW32__
#	ifndef IPV6_V6ONLY
#		define IPV6_V6ONLY 27
#	endif /* IPV6_V6ONLY */
#endif /* __MINGW32__ */

typedef int socklen_t;
typedef SSIZE_T ssize_t;

#define MAXPATHLEN MAX_PATH

#ifdef _MSC_VER
typedef DWORD pid_t;
#define strcasecmp stricmp
#endif /* _MSC_VER */

#endif /* WIN32_H */
