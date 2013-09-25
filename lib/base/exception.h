/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/i2-base.h"
#include "base/qstring.h"
#include "base/stacktrace.h"
#include <sstream>
#include <boost/thread/tss.hpp>
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>

#ifdef _WIN32
#	include <boost/algorithm/string/trim.hpp>
#endif /* _WIN32 */

namespace icinga
{

/**
 * Base class for all exceptions.
 *
 * @ingroup base
 */
class I2_BASE_API Exception
{
public:
	static StackTrace *GetLastStackTrace(void);
	static void SetLastStackTrace(const StackTrace& trace);

private:
	static boost::thread_specific_ptr<StackTrace> m_LastStackTrace;
};

typedef boost::error_info<StackTrace, StackTrace> StackTraceErrorInfo;

class I2_BASE_API posix_error : virtual public std::exception, virtual public boost::exception { };

#ifdef _WIN32
class I2_BASE_API win32_error : virtual public std::exception, virtual public boost::exception { };

struct errinfo_win32_error_;
typedef boost::error_info<struct errinfo_win32_error_, int> errinfo_win32_error;

inline std::string to_string(const errinfo_win32_error& e)
{
	std::ostringstream tmp;
	int code = e.value();

	char *message;
	String result = "Unknown error.";

	DWORD rc = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, (char *)&message,
		0, NULL);

	if (rc != 0) {
		result = String(message);
		LocalFree(message);

		/* remove trailing new-line characters */
		boost::algorithm::trim_right(result);
	}

	tmp << code << ", \"" << result << "\"";
	return tmp.str();
}
#endif /* _WIN32 */

struct errinfo_getaddrinfo_error_;
typedef boost::error_info<struct errinfo_getaddrinfo_error_, int> errinfo_getaddrinfo_error;

inline std::string to_string(const errinfo_getaddrinfo_error& e)
{
	return gai_strerror(e.value());
}

}

#endif /* EXCEPTION_H */
