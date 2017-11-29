/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

/**
 * @mainpage Icinga Documentation
 *
 * Icinga implements a framework for run-time-loadable components which can
 * pass messages between each other. These components can either be hosted in
 * the same process or in several host processes (either on the same machine or
 * on different machines).
 *
 * The framework's code critically depends on the following patterns:
 *
 * <list type="bullet">
 * <item>Smart pointers
 *
 * The shared_ptr and weak_ptr template classes are used to simplify memory
 * management and to avoid accidental memory leaks and use-after-free
 * bugs.</item>
 *
 * <item>Observer pattern
 *
 * Framework classes expose events which other objects can subscribe to. This
 * is used to decouple clients of a class from the class' internal
 * implementation.</item>
 * </list>
 */

/**
 * @defgroup base Base class library
 *
 * The base class library implements commonly-used functionality like
 * event handling for sockets and timers.
 */

#include <boost/config.hpp>

#if defined(__clang__) && __cplusplus >= 201103L
#	undef BOOST_NO_CXX11_HDR_TUPLE
#endif

#ifdef _MSC_VER
#	pragma warning(disable:4251)
#	pragma warning(disable:4275)
#	pragma warning(disable:4345)
#endif /* _MSC_VER */

#include "config.h"

#ifdef _WIN32
#	include "base/win32.hpp"
#else
#	include "base/unix.hpp"
#endif

#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cerrno>

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

#include <exception>
#include <stdexcept>

#if defined(__APPLE__) && defined(__MACH__)
#	pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include "base/visibility.hpp"

#ifdef I2_BASE_BUILD
#	define I2_BASE_API I2_EXPORT
#else /* I2_BASE_BUILD */
#	define I2_BASE_API I2_IMPORT
#endif /* I2_BASE_BUILD */

#if defined(__GNUC__)
#	define likely(x) __builtin_expect(!!(x), 1)
#	define unlikely(x) __builtin_expect(!!(x), 0)
#else
#	define likely(x) (x)
#	define unlikely(x) (x)
#endif

#define BOOST_BIND_NO_PLACEHOLDERS

#include <functional>

using namespace std::placeholders;

#endif /* I2BASE_H */
