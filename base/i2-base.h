/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#ifndef I2BASE_H
#define I2BASE_H

#ifdef _MSC_VER
#	define HAVE_CXX11
#	pragma warning(disable:4251)
#	define _CRT_SECURE_NO_DEPRECATE
#	define _CRT_SECURE_NO_WARNINGS
#else /* _MSC_VER */
#	include "config.h"
#endif /* _MSC_VER */

#define PLATFORM_WINDOWS 1
#define PLATFORM_UNIX 2

#ifdef _WIN32
#	define I2_PLATFORM PLATFORM_WINDOWS
#	include "win32.h"
#else
#	define I2_PLATFORM PLATFORM_UNIX
#	include "unix.h"
#endif

#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cerrno>

#include <memory>
#include <string>
#include <sstream>
#include <vector>
#include <set>
#include <iostream>
#include <list>
#include <typeinfo>
#include <map>
#include <list>
#include <algorithm>
#include <functional>

#if defined(__APPLE__) && defined(__MACH__)
#	pragma GCC diagnostic ignored "-Wdeprecated-declarations" 
#endif

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef HAVE_GCC_ABI_DEMANGLE
#	include <cxxabi.h>
#endif /* HAVE_GCC_ABI_DEMANGLE */

using namespace std;

#ifdef HAVE_CXX11
#	include <memory>
#	include <functional>

using namespace std::placeholders;
#else
#	include <tr1/memory>
#	include <tr1/functional>

using namespace std::tr1;
using namespace std::tr1::placeholders;

#	include "cxx11-compat.h"
#endif

#ifdef I2_BASE_BUILD
#	define I2_BASE_API I2_EXPORT
#else /* I2_BASE_BUILD */
#	define I2_BASE_API I2_IMPORT
#endif /* I2_BASE_BUILD */

#include "mutex.h"
#include "lock.h"
#include "condvar.h"
#include "thread.h"
#include "utility.h"
#include "object.h"
#include "exception.h"
#include "memory.h"
#include "delegate.h"
#include "observable.h"
#include "variant.h"
#include "dictionary.h"
#include "timer.h"
#include "fifo.h"
#include "socket.h"
#include "tcpsocket.h"
#include "tcpclient.h"
#include "tcpserver.h"
#include "tlsclient.h"
#include "configobject.h"
#include "configcollection.h"
#include "confighive.h"
#include "application.h"
#include "component.h"

#endif /* I2BASE_H */
