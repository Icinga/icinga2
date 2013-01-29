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

#ifdef _MSC_VER
#	pragma warning(disable:4251)
#	pragma warning(disable:4275)
#	pragma warning(disable:4345)
#	define _CRT_SECURE_NO_DEPRECATE
#	define _CRT_SECURE_NO_WARNINGS
#else /* _MSC_VER */
#	include "config.h"
#endif /* _MSC_VER */

#ifdef _WIN32
#	include "win32.h"
#else
#	include "unix.h"
#endif

#include <cstdlib>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <cerrno>

#include <string>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <set>
#include <iostream>
#include <fstream>
#include <list>
#include <typeinfo>
#include <map>
#include <list>
#include <algorithm>
#include <deque>

using std::vector;
using std::map;
using std::list;
using std::set;
using std::multimap;
using std::multiset;
using std::pair;
using std::deque;
using std::make_pair;

using std::stringstream;
using std::istream;
using std::ostream;
using std::fstream;
using std::ifstream;
using std::ofstream;
using std::iostream;

using std::exception;
using std::bad_alloc;
using std::bad_cast;
using std::runtime_error;
using std::logic_error;
using std::invalid_argument;
using std::domain_error;

using std::type_info;

#include <boost/smart_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/signal.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/thread.hpp>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

using boost::shared_ptr;
using boost::weak_ptr;
using boost::enable_shared_from_this;
using boost::dynamic_pointer_cast;
using boost::static_pointer_cast;
using boost::function;
using boost::thread;
using boost::thread_group;
using boost::condition_variable;
using boost::system_time;
using boost::posix_time::millisec;
using boost::tie;
using boost::throw_exception;
using boost::rethrow_exception;
using boost::current_exception;

namespace tuples = boost::tuples;

#if defined(__APPLE__) && defined(__MACH__)
#	pragma GCC diagnostic ignored "-Wdeprecated-declarations" 
#endif

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#ifdef HAVE_GCC_ABI_DEMANGLE
#	include <cxxabi.h>
#endif /* HAVE_GCC_ABI_DEMANGLE */

#ifdef I2_BASE_BUILD
#	define I2_BASE_API I2_EXPORT
#else /* I2_BASE_BUILD */
#	define I2_BASE_API I2_IMPORT
#endif /* I2_BASE_BUILD */

#include "qstring.h"
#include "utility.h"
#include "object.h"
#include "exception.h"
#include "event.h"
#include "value.h"
#include "convert.h"
#include "dictionary.h"
#include "ringbuffer.h"
#include "timer.h"
#include "stream.h"
#include "stream_bio.h"
#include "connection.h"
#include "netstring.h"
#include "fifo.h"
#include "stdiostream.h"
#include "socket.h"
#include "tcpsocket.h"
#include "unixsocket.h"
#include "tlsstream.h"
#include "asynctask.h"
#include "process.h"
#include "scriptfunction.h"
#include "scripttask.h"
#include "dynamicobject.h"
#include "dynamictype.h"
#include "logger.h"
#include "application.h"
#include "component.h"
#include "streamlogger.h"
#include "sysloglogger.h"

#endif /* I2BASE_H */
