/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EXCEPTION_H
#define EXCEPTION_H

#include "base/i2-base.hpp"
#include "base/string.hpp"
#include "base/context.hpp"
#include "base/debuginfo.hpp"
#include "base/dictionary.hpp"
#include "base/configobject.hpp"
#include <boost/exception/errinfo_api_function.hpp>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/exception/errinfo_file_name.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/exception_ptr.hpp>
#include <boost/stacktrace.hpp>

#ifdef _WIN32
#	include <boost/algorithm/string/trim.hpp>
#endif /* _WIN32 */

namespace icinga
{

class user_error : virtual public std::exception, virtual public boost::exception
{ };

/*
 * @ingroup base
 */
class ScriptError : virtual public user_error
{
public:
	ScriptError(String message);
	ScriptError(String message, DebugInfo di, bool incompleteExpr = false);

	const char *what(void) const throw() final;

	DebugInfo GetDebugInfo() const;
	bool IsIncompleteExpression() const;

	bool IsHandledByDebugger() const;
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
class ValidationError : virtual public user_error
{
public:
	ValidationError(const ConfigObject::Ptr& object, const std::vector<String>& attributePath, const String& message);
	~ValidationError() throw() override;

	const char *what() const throw() override;

	ConfigObject::Ptr GetObject() const;
	std::vector<String> GetAttributePath() const;
	String GetMessage() const;

	void SetDebugHint(const Dictionary::Ptr& dhint);
	Dictionary::Ptr GetDebugHint() const;

private:
	ConfigObject::Ptr m_Object;
	std::vector<String> m_AttributePath;
	String m_Message;
	String m_What;
	Dictionary::Ptr m_DebugHint;
};

boost::stacktrace::stacktrace *GetLastExceptionStack();
void SetLastExceptionStack(const boost::stacktrace::stacktrace& trace);

ContextTrace *GetLastExceptionContext();
void SetLastExceptionContext(const ContextTrace& context);

void RethrowUncaughtException();

struct errinfo_stacktrace_;
typedef boost::error_info<struct errinfo_stacktrace_, boost::stacktrace::stacktrace> StackTraceErrorInfo;

std::string to_string(const StackTraceErrorInfo&);

typedef boost::error_info<ContextTrace, ContextTrace> ContextTraceErrorInfo;

std::string to_string(const ContextTraceErrorInfo& e);

/**
 * Generate diagnostic information about an exception
 *
 * The following information is gathered in the result:
 *  - Exception error message
 *  - Debug information about the Icinga config if the exception is a ValidationError
 *  - Stack trace
 *  - Context trace
 *
 *  Each, stack trace and the context trace, are printed if the they were saved in the boost exception error
 *  information, are explicitly passed as a parameter, or were stored when the last exception was thrown. If multiple
 *  of these exist, the first one is used.
 *
 * @param ex exception to print diagnostic information about
 * @param verbose if verbose is set, a stack trace is added
 * @param stack optionally supply a stack trace
 * @param context optionally supply a context trace
 * @return string containing the aforementioned information
 */
String DiagnosticInformation(const std::exception& ex, bool verbose = true,
	boost::stacktrace::stacktrace *stack = nullptr, ContextTrace *context = nullptr);
String DiagnosticInformation(const boost::exception_ptr& eptr, bool verbose = true);

class posix_error : virtual public std::exception, virtual public boost::exception {
public:
	~posix_error() throw() override;

	const char *what(void) const throw() final;

private:
	mutable char *m_Message{nullptr};
};

#ifdef _WIN32
class win32_error : virtual public std::exception, virtual public boost::exception {
public:
	const char *what() const noexcept override;
};

std::string to_string(const win32_error& e);

struct errinfo_win32_error_;
typedef boost::error_info<struct errinfo_win32_error_, int> errinfo_win32_error;

std::string to_string(const errinfo_win32_error& e);
#endif /* _WIN32 */

struct errinfo_getaddrinfo_error_;
typedef boost::error_info<struct errinfo_getaddrinfo_error_, int> errinfo_getaddrinfo_error;

std::string to_string(const errinfo_getaddrinfo_error& e);

struct errinfo_message_;
typedef boost::error_info<struct errinfo_message_, std::string> errinfo_message;

class invalid_downtime_removal_error : virtual public std::exception, virtual public boost::exception {
public:
	explicit invalid_downtime_removal_error(String message);
	explicit invalid_downtime_removal_error(const char* message);

	~invalid_downtime_removal_error() noexcept override;

	const char *what() const noexcept final;

private:
	String m_Message;
};

}

#endif /* EXCEPTION_H */
