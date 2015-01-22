/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include "base/stacktrace.hpp"
#include "base/context.hpp"
#include "base/utility.hpp"
#include <sstream>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>

#ifdef _WIN32
#	include <boost/algorithm/string/trim.hpp>
#endif /* _WIN32 */

namespace icinga
{

class I2_BASE_API user_error : virtual public std::exception, virtual public boost::exception
{ };

I2_BASE_API StackTrace *GetLastExceptionStack(void);
I2_BASE_API void SetLastExceptionStack(const StackTrace& trace);

I2_BASE_API ContextTrace *GetLastExceptionContext(void);
I2_BASE_API void SetLastExceptionContext(const ContextTrace& context);

I2_BASE_API void RethrowUncaughtException(void);

typedef boost::error_info<StackTrace, StackTrace> StackTraceErrorInfo;
typedef boost::error_info<ContextTrace, ContextTrace> ContextTraceErrorInfo;

template<typename T>
String DiagnosticInformation(const T& ex, StackTrace *stack = NULL, ContextTrace *context = NULL)
{
	std::ostringstream result;

	result << boost::diagnostic_information(ex);

	if (dynamic_cast<const user_error *>(&ex) == NULL) {
		if (boost::get_error_info<StackTraceErrorInfo>(ex) == NULL) {
			result << std::endl;

			if (!stack)
				stack = GetLastExceptionStack();
			
			if (stack)
				result << *stack;
				
		}

		if (boost::get_error_info<ContextTraceErrorInfo>(ex) == NULL) {
			result << std::endl;

			if (!context)
				context = GetLastExceptionContext();

			if (context)
				result << *context;
		}
	}

	return result.str();
}

I2_BASE_API String DiagnosticInformation(boost::exception_ptr eptr);

class I2_BASE_API posix_error : virtual public std::exception, virtual public boost::exception { };

#ifdef _WIN32
class I2_BASE_API win32_error : virtual public std::exception, virtual public boost::exception { };

struct errinfo_win32_error_;
typedef boost::error_info<struct errinfo_win32_error_, int> errinfo_win32_error;

inline std::string to_string(const errinfo_win32_error& e)
{
	return Utility::FormatErrorNumber(e.value());
}
#endif /* _WIN32 */

struct errinfo_getaddrinfo_error_;
typedef boost::error_info<struct errinfo_getaddrinfo_error_, int> errinfo_getaddrinfo_error;

inline std::string to_string(const errinfo_getaddrinfo_error& e)
{
	return gai_strerror(e.value());
}

struct errinfo_message_;
typedef boost::error_info<struct errinfo_message_, std::string> errinfo_message;

}

#endif /* EXCEPTION_H */
