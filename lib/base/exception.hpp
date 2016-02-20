/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include "base/debuginfo.hpp"
#include "base/dictionary.hpp"
#include "base/configobject.hpp"
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

/*
 * @ingroup base
 */
class I2_BASE_API ScriptError : virtual public user_error
{
public:
	ScriptError(const String& message);
	ScriptError(const String& message, const DebugInfo& di, bool incompleteExpr = false);
	~ScriptError(void) throw();

	virtual const char *what(void) const throw() override;

	DebugInfo GetDebugInfo(void) const;
	bool IsIncompleteExpression(void) const;

	bool IsHandledByDebugger(void) const;
	void SetHandledByDebugger(bool handled);

private:
	String m_Message;
	DebugInfo m_DebugInfo;
	bool m_IncompleteExpr;
	bool m_HandledByDebugger;
};

/*
 * @ingroup base
 */
class I2_BASE_API ValidationError : virtual public user_error
{
public:
	ValidationError(const ConfigObject::Ptr& object, const std::vector<String>& attributePath, const String& message);
	~ValidationError(void) throw();

	virtual const char *what(void) const throw() override;

	ConfigObject::Ptr GetObject(void) const;
	std::vector<String> GetAttributePath(void) const;
	String GetMessage(void) const;

	void SetDebugHint(const Dictionary::Ptr& dhint);
	Dictionary::Ptr GetDebugHint(void) const;

private:
	ConfigObject::Ptr m_Object;
	std::vector<String> m_AttributePath;
	String m_Message;
	String m_What;
	Dictionary::Ptr m_DebugHint;
};

I2_BASE_API StackTrace *GetLastExceptionStack(void);
I2_BASE_API void SetLastExceptionStack(const StackTrace& trace);

I2_BASE_API ContextTrace *GetLastExceptionContext(void);
I2_BASE_API void SetLastExceptionContext(const ContextTrace& context);

I2_BASE_API void RethrowUncaughtException(void);

typedef boost::error_info<StackTrace, StackTrace> StackTraceErrorInfo;

inline std::string to_string(const StackTraceErrorInfo& e)
{
	return "";
}

typedef boost::error_info<ContextTrace, ContextTrace> ContextTraceErrorInfo;

inline std::string to_string(const ContextTraceErrorInfo& e)
{
	std::ostringstream msgbuf;
	msgbuf << "[Context] = " << e.value();
	return msgbuf.str();
}

I2_BASE_API String DiagnosticInformation(const std::exception& ex, bool verbose = true, StackTrace *stack = NULL, ContextTrace *context = NULL);
I2_BASE_API String DiagnosticInformation(boost::exception_ptr eptr, bool verbose = true);

class I2_BASE_API posix_error : virtual public std::exception, virtual public boost::exception {
public:
	posix_error(void);
	virtual ~posix_error(void) throw();

	virtual const char *what(void) const throw() override;

private:
	mutable char *m_Message;
};

#ifdef _WIN32
class I2_BASE_API win32_error : virtual public std::exception, virtual public boost::exception { };

struct errinfo_win32_error_;
typedef boost::error_info<struct errinfo_win32_error_, int> errinfo_win32_error;

inline std::string to_string(const errinfo_win32_error& e)
{
	return "[errinfo_win32_error] = " + Utility::FormatErrorNumber(e.value()) + "\n";
}
#endif /* _WIN32 */

struct errinfo_getaddrinfo_error_;
typedef boost::error_info<struct errinfo_getaddrinfo_error_, int> errinfo_getaddrinfo_error;

inline std::string to_string(const errinfo_getaddrinfo_error& e)
{
	String msg;

#ifdef _WIN32
	msg = gai_strerrorA(e.value());
#else /* _WIN32 */
	msg = gai_strerror(e.value());
#endif /* _WIN32 */

	return "[errinfo_getaddrinfo_error] = " + String(msg) + "\n";
}

struct errinfo_message_;
typedef boost::error_info<struct errinfo_message_, std::string> errinfo_message;

}

#endif /* EXCEPTION_H */
