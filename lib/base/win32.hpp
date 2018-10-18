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

#ifndef WIN32_H
#define WIN32_H

#define WIN32_LEAN_AND_MEAN
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_VISTA
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
