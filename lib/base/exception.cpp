/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/exception.hpp"
#include "base/stacktrace.hpp"
#include <boost/thread/tss.hpp>
#include <utility>

#ifdef _WIN32
#	include "base/utility.hpp"
#endif /* _WIN32 */

#ifdef HAVE_CXXABI_H
#	include <cxxabi.h>
#endif /* HAVE_CXXABI_H */

using namespace icinga;

static boost::thread_specific_ptr<boost::stacktrace::stacktrace> l_LastExceptionStack;
static boost::thread_specific_ptr<ContextTrace> l_LastExceptionContext;

#ifdef HAVE_CXXABI_H

#ifdef _LIBCPPABI_VERSION
class libcxx_type_info : public std::type_info
{
public:
	~libcxx_type_info() override;

	virtual void noop1() const;
	virtual void noop2() const;
	virtual bool can_catch(const libcxx_type_info *thrown_type, void *&adjustedPtr) const = 0;
};
#endif /* _LIBCPPABI_VERSION */


#if defined(__GLIBCXX__) || defined(_LIBCPPABI_VERSION)
/**
 * Attempts to cast an exception to a destination type
 *
 * @param obj Exception to be casted
 * @param src Type information of obj
 * @param dst Information of which type to cast to
 * @return Pointer to the exception if the cast is possible, nullptr otherwise
 */
inline void *cast_exception(void *obj, const std::type_info *src, const std::type_info *dst)
{
#ifdef __GLIBCXX__
	void *thrown_ptr = obj;

	/* Check if the exception is a pointer type. */
	if (src->__is_pointer_p())
		thrown_ptr = *(void **)thrown_ptr;

	if (dst->__do_catch(src, &thrown_ptr, 1))
		return thrown_ptr;
	else
		return nullptr;
#else /* __GLIBCXX__ */
	const auto *srcInfo = static_cast<const libcxx_type_info *>(src);
	const auto *dstInfo = static_cast<const libcxx_type_info *>(dst);

	void *adj = obj;

	if (dstInfo->can_catch(srcInfo, adj))
		return adj;
	else
		return nullptr;
#endif /* __GLIBCXX__ */

}
#else /* defined(__GLIBCXX__) || defined(_LIBCPPABI_VERSION) */
#define NO_CAST_EXCEPTION
#endif /* defined(__GLIBCXX__) || defined(_LIBCPPABI_VERSION) */

#	if __clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ > 3)
#		define TYPEINFO_TYPE std::type_info
#	else
#		define TYPEINFO_TYPE void
#	endif

#	if !defined(__GLIBCXX__) && !defined(_WIN32)
static boost::thread_specific_ptr<void *> l_LastExceptionObj;
static boost::thread_specific_ptr<TYPEINFO_TYPE *> l_LastExceptionPvtInfo;

typedef void (*DestCallback)(void *);
static boost::thread_specific_ptr<DestCallback> l_LastExceptionDest;
#	endif /* !__GLIBCXX__ && !_WIN32 */

extern "C" void __cxa_throw(void *obj, TYPEINFO_TYPE *pvtinfo, void (*dest)(void *));
#endif /* HAVE_CXXABI_H */

void icinga::RethrowUncaughtException()
{
#if defined(__GLIBCXX__) || !defined(HAVE_CXXABI_H)
	throw;
#else /* __GLIBCXX__ || !HAVE_CXXABI_H */
	__cxa_throw(*l_LastExceptionObj.get(), *l_LastExceptionPvtInfo.get(), *l_LastExceptionDest.get());
#endif /* __GLIBCXX__ || !HAVE_CXXABI_H */
}

#ifdef HAVE_CXXABI_H
extern "C"
void __cxa_throw(void *obj, TYPEINFO_TYPE *pvtinfo, void (*dest)(void *))
{
	/* This function overrides an internal function of libstdc++ that is called when a C++ exception is thrown in order
	 * to capture as much information as possible at that time and then call the original implementation. This
	 * information includes:
	 *  - stack trace (for later use in DiagnosticInformation)
	 *  - context trace (for later use in DiagnosticInformation)
	 */

	auto *tinfo = static_cast<std::type_info *>(pvtinfo);

	typedef void (*cxa_throw_fn)(void *, std::type_info *, void (*)(void *)) __attribute__((noreturn));
	static cxa_throw_fn real_cxa_throw;

#if !defined(__GLIBCXX__) && !defined(_WIN32)
	l_LastExceptionObj.reset(new void *(obj));
	l_LastExceptionPvtInfo.reset(new TYPEINFO_TYPE *(pvtinfo));
	l_LastExceptionDest.reset(new DestCallback(dest));
#endif /* !defined(__GLIBCXX__) && !defined(_WIN32) */

	// resolve symbol to original implementation of __cxa_throw for the call at the end of this function
	if (real_cxa_throw == nullptr)
		real_cxa_throw = (cxa_throw_fn)dlsym(RTLD_NEXT, "__cxa_throw");

#ifndef NO_CAST_EXCEPTION
	void *uex = cast_exception(obj, tinfo, &typeid(user_error));
	auto *ex = reinterpret_cast<boost::exception *>(cast_exception(obj, tinfo, &typeid(boost::exception)));

	if (!uex) {
#endif /* NO_CAST_EXCEPTION */
		// save the current stack trace in a thread-local variable
		boost::stacktrace::stacktrace stack;
		SetLastExceptionStack(stack);

#ifndef NO_CAST_EXCEPTION
		// save the current stack trace in the boost exception error info if the exception is a boost::exception
		if (ex && !boost::get_error_info<StackTraceErrorInfo>(*ex))
			*ex << StackTraceErrorInfo(stack);
	}
#endif /* NO_CAST_EXCEPTION */

	ContextTrace context;
	SetLastExceptionContext(context);

#ifndef NO_CAST_EXCEPTION
	// save the current context trace in the boost exception error info if the exception is a boost::exception
	if (ex && !boost::get_error_info<ContextTraceErrorInfo>(*ex))
		*ex << ContextTraceErrorInfo(context);
#endif /* NO_CAST_EXCEPTION */

	real_cxa_throw(obj, tinfo, dest);
}
#endif /* HAVE_CXXABI_H */

boost::stacktrace::stacktrace *icinga::GetLastExceptionStack()
{
	return l_LastExceptionStack.get();
}

void icinga::SetLastExceptionStack(const boost::stacktrace::stacktrace& trace)
{
	l_LastExceptionStack.reset(new boost::stacktrace::stacktrace(trace));
}

ContextTrace *icinga::GetLastExceptionContext()
{
	return l_LastExceptionContext.get();
}

void icinga::SetLastExceptionContext(const ContextTrace& context)
{
	l_LastExceptionContext.reset(new ContextTrace(context));
}

String icinga::DiagnosticInformation(const std::exception& ex, bool verbose, boost::stacktrace::stacktrace *stack, ContextTrace *context)
{
	std::ostringstream result;

	String message = ex.what();

#ifdef _WIN32
	const auto *win32_err = dynamic_cast<const win32_error *>(&ex);
	if (win32_err) {
		message = to_string(*win32_err);
	}
#endif /* _WIN32 */

	const auto *vex = dynamic_cast<const ValidationError *>(&ex);

	if (message.IsEmpty())
		result << boost::diagnostic_information(ex) << "\n";
	else
		result << "Error: " << message << "\n";

	const auto *dex = dynamic_cast<const ScriptError *>(&ex);

	if (dex && !dex->GetDebugInfo().Path.IsEmpty())
		ShowCodeLocation(result, dex->GetDebugInfo());

	if (vex) {
		DebugInfo di;

		ConfigObject::Ptr dobj = vex->GetObject();
		if (dobj)
			di = dobj->GetDebugInfo();

		Dictionary::Ptr currentHint = vex->GetDebugHint();
		Array::Ptr messages;

		if (currentHint) {
			for (const String& attr : vex->GetAttributePath()) {
				Dictionary::Ptr props = currentHint->Get("properties");

				if (!props)
					break;

				currentHint = props->Get(attr);

				if (!currentHint)
					break;

				messages = currentHint->Get("messages");
			}
		}

		if (messages && messages->GetLength() > 0) {
			Array::Ptr message = messages->Get(messages->GetLength() - 1);

			di.Path = message->Get(1);
			di.FirstLine = message->Get(2);
			di.FirstColumn = message->Get(3);
			di.LastLine = message->Get(4);
			di.LastColumn = message->Get(5);
		}

		if (!di.Path.IsEmpty())
			ShowCodeLocation(result, di);
	}

	const auto *uex = dynamic_cast<const user_error *>(&ex);
	const auto *pex = dynamic_cast<const posix_error *>(&ex);

	if (!uex && !pex && verbose) {
		// Print the first of the following stack traces (if any exists)
		//   1. stack trace from boost exception error information
		const boost::stacktrace::stacktrace *st = boost::get_error_info<StackTraceErrorInfo>(ex);
		//   2. stack trace explicitly passed as a parameter
		if (!st) {
			st = stack;
		}
		//   3. stack trace saved when the last exception was thrown
		if (!st) {
			st = GetLastExceptionStack();
		}

		if (st && !st->empty()) {
			result << "\nStacktrace:\n" << StackTraceFormatter(*st);
		}
	}

	// Print the first of the following context traces (if any exists)
	//   1. context trace from boost exception error information
	const ContextTrace *ct = boost::get_error_info<ContextTraceErrorInfo>(ex);
	//   2. context trace explicitly passed as a parameter
	if (!ct) {
		ct = context;
	}
	//   3. context trace saved when the last exception was thrown
	if (!ct) {
		ct = GetLastExceptionContext();
	}

	if (ct && ct->GetLength() > 0) {
		result << "\nContext:\n" << *ct;
	}

	return result.str();
}

String icinga::DiagnosticInformation(const boost::exception_ptr& eptr, bool verbose)
{
	boost::stacktrace::stacktrace *pt = GetLastExceptionStack();
	boost::stacktrace::stacktrace stack;

	ContextTrace *pc = GetLastExceptionContext();
	ContextTrace context;

	if (pt)
		stack = *pt;

	if (pc)
		context = *pc;

	try {
		boost::rethrow_exception(eptr);
	} catch (const std::exception& ex) {
		return DiagnosticInformation(ex, verbose, pt ? &stack : nullptr, pc ? &context : nullptr);
	}

	return boost::diagnostic_information(eptr);
}

ScriptError::ScriptError(String message)
	: m_Message(std::move(message)), m_IncompleteExpr(false)
{ }

ScriptError::ScriptError(String message, DebugInfo di, bool incompleteExpr)
	: m_Message(std::move(message)), m_DebugInfo(std::move(di)), m_IncompleteExpr(incompleteExpr), m_HandledByDebugger(false)
{ }

const char *ScriptError::what() const throw()
{
	return m_Message.CStr();
}

DebugInfo ScriptError::GetDebugInfo() const
{
	return m_DebugInfo;
}

bool ScriptError::IsIncompleteExpression() const
{
	return m_IncompleteExpr;
}

bool ScriptError::IsHandledByDebugger() const
{
	return m_HandledByDebugger;
}

void ScriptError::SetHandledByDebugger(bool handled)
{
	m_HandledByDebugger = handled;
}

posix_error::~posix_error() throw()
{
	free(m_Message);
}

const char *posix_error::what() const throw()
{
	if (!m_Message) {
		std::ostringstream msgbuf;

		const char * const *func = boost::get_error_info<boost::errinfo_api_function>(*this);

		if (func)
			msgbuf << "Function call '" << *func << "'";
		else
			msgbuf << "Function call";

		const std::string *fname = boost::get_error_info<boost::errinfo_file_name>(*this);

		if (fname)
			msgbuf << " for file '" << *fname << "'";

		msgbuf << " failed";

		const int *errnum = boost::get_error_info<boost::errinfo_errno>(*this);

		if (errnum)
			msgbuf << " with error code " << *errnum << ", '" << strerror(*errnum) << "'";

		String str = msgbuf.str();
		m_Message = strdup(str.CStr());
	}

	return m_Message;
}

ValidationError::ValidationError(const ConfigObject::Ptr& object, const std::vector<String>& attributePath, const String& message)
	: m_Object(object), m_AttributePath(attributePath), m_Message(message)
{
	String path;

	for (const String& attribute : attributePath) {
		if (!path.IsEmpty())
			path += " -> ";

		path += "'" + attribute + "'";
	}

	Type::Ptr type = object->GetReflectionType();
	m_What = "Validation failed for object '" + object->GetName() + "' of type '" + type->GetName() + "'";

	if (!path.IsEmpty())
		m_What += "; Attribute " + path;

	m_What += ": " + message;
}

ValidationError::~ValidationError() throw()
{ }

const char *ValidationError::what() const throw()
{
	return m_What.CStr();
}

ConfigObject::Ptr ValidationError::GetObject() const
{
	return m_Object;
}

std::vector<String> ValidationError::GetAttributePath() const
{
	return m_AttributePath;
}

String ValidationError::GetMessage() const
{
	return m_Message;
}

void ValidationError::SetDebugHint(const Dictionary::Ptr& dhint)
{
	m_DebugHint = dhint;
}

Dictionary::Ptr ValidationError::GetDebugHint() const
{
	return m_DebugHint;
}

std::string icinga::to_string(const StackTraceErrorInfo&)
{
	return "";
}

#ifdef _WIN32
const char *win32_error::what() const noexcept
{
	return "win32_error";
}

std::string icinga::to_string(const win32_error &e) {
	std::ostringstream msgbuf;

	const char * const *func = boost::get_error_info<boost::errinfo_api_function>(e);

	if (func) {
		msgbuf << "Function call '" << *func << "'";
	} else {
		msgbuf << "Function call";
	}

	const std::string *fname = boost::get_error_info<boost::errinfo_file_name>(e);

	if (fname) {
		msgbuf << " for file '" << *fname << "'";
	}

	msgbuf << " failed";

	const int *errnum = boost::get_error_info<errinfo_win32_error>(e);

	if (errnum) {
		msgbuf << " with error code " << Utility::FormatErrorNumber(*errnum);
	}

	return msgbuf.str();
}

std::string icinga::to_string(const errinfo_win32_error& e)
{
	return "[errinfo_win32_error] = " + Utility::FormatErrorNumber(e.value()) + "\n";
}
#endif /* _WIN32 */

std::string icinga::to_string(const errinfo_getaddrinfo_error& e)
{
	String msg;

#ifdef _WIN32
	msg = gai_strerrorA(e.value());
#else /* _WIN32 */
	msg = gai_strerror(e.value());
#endif /* _WIN32 */

	return "[errinfo_getaddrinfo_error] = " + String(msg) + "\n";
}

std::string icinga::to_string(const ContextTraceErrorInfo& e)
{
	std::ostringstream msgbuf;
	msgbuf << "[Context] = " << e.value();
	return msgbuf.str();
}

invalid_downtime_removal_error::invalid_downtime_removal_error(String message)
	: m_Message(std::move(message))
{ }

invalid_downtime_removal_error::invalid_downtime_removal_error(const char *message)
	: m_Message(message)
{ }

invalid_downtime_removal_error::~invalid_downtime_removal_error() noexcept
{ }

const char *invalid_downtime_removal_error::what() const noexcept
{
	return m_Message.CStr();
}
