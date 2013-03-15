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

#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>

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
#include <stack>
#include <iterator>

using std::vector;
using std::map;
using std::list;
using std::set;
using std::multimap;
using std::multiset;
using std::pair;
using std::deque;
using std::stack;
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
#include <boost/signals2.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/compare.hpp>
#include <boost/thread.hpp>
#include <boost/variant.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/program_options.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception/error_info.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/key_extractors.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

using boost::shared_ptr;
using boost::weak_ptr;
using boost::enable_shared_from_this;
using boost::dynamic_pointer_cast;
using boost::static_pointer_cast;
using boost::function;
using boost::thread;
using boost::thread_group;
using boost::recursive_mutex;
using boost::condition_variable;
using boost::system_time;
using boost::posix_time::millisec;
using boost::tie;
using boost::rethrow_exception;
using boost::current_exception;
using boost::diagnostic_information;
using boost::errinfo_api_function;
using boost::errinfo_errno;
using boost::errinfo_file_name;
using boost::multi_index_container;
using boost::multi_index::indexed_by;
using boost::multi_index::identity;
using boost::multi_index::ordered_unique;
using boost::multi_index::ordered_non_unique;
using boost::multi_index::nth_index;

namespace tuples = boost::tuples;
namespace signals2 = boost::signals2;

#if defined(__APPLE__) && defined(__MACH__)
#	pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#endif

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#if HAVE_GCC_ABI_DEMANGLE
#	include <cxxabi.h>
#endif /* HAVE_GCC_ABI_DEMANGLE */

#ifdef I2_BASE_BUILD
#	define I2_BASE_API I2_EXPORT
#else /* I2_BASE_BUILD */
#	define I2_BASE_API I2_IMPORT
#endif /* I2_BASE_BUILD */

#include "qstring.h"
#include "utility.h"
#include "stacktrace.h"
#include "object.h"
#include "objectlock.h"
#include "exception.h"
#include "eventqueue.h"
#include "value.h"
#include "convert.h"
#include "dictionary.h"
#include "array.h"
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
#include "attribute.h"
#include "dynamicobject.h"
#include "dynamictype.h"
#include "script.h"
#include "scriptinterpreter.h"
#include "scriptlanguage.h"
#include "logger.h"
#include "application.h"
#include "streamlogger.h"
#include "sysloglogger.h"

#endif /* I2BASE_H */
