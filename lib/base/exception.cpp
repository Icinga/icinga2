/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/exception.hpp"

#ifdef HAVE_CXXABI_H
#	include <cxxabi.h>
#endif /* HAVE_CXXABI_H */

using namespace icinga;

static boost::thread_specific_ptr<StackTrace> l_LastExceptionStack;
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
	auto *tinfo = static_cast<std::type_info *>(pvtinfo);

	typedef void (*cxa_throw_fn)(void *, std::type_info *, void (*)(void *)) __attribute__((noreturn));
	static cxa_throw_fn real_cxa_throw;

#if !defined(__GLIBCXX__) && !defined(_WIN32)
	l_LastExceptionObj.reset(new void *(obj));
	l_LastExceptionPvtInfo.reset(new TYPEINFO_TYPE *(pvtinfo));
	l_LastExceptionDest.reset(new DestCallback(dest));
#endif /* !defined(__GLIBCXX__) && !defined(_WIN32) */

	if (real_cxa_throw == nullptr)
		real_cxa_throw = (cxa_throw_fn)dlsym(RTLD_NEXT, "__cxa_throw");

#ifndef NO_CAST_EXCEPTION
	void *uex = cast_exception(obj, tinfo, &typeid(user_error));
	auto *ex = reinterpret_cast<boost::exception *>(cast_exception(obj, tinfo, &typeid(boost::exception)));

	if (!uex) {
#endif /* NO_CAST_EXCEPTION */
		StackTrace stack;
		SetLastExceptionStack(stack);

#ifndef NO_CAST_EXCEPTION
		if (ex && !boost::get_error_info<StackTraceErrorInfo>(*ex))
			*ex << StackTraceErrorInfo(stack);
	}
#endif /* NO_CAST_EXCEPTION */

	ContextTrace context;
	SetLastExceptionContext(context);

#ifndef NO_CAST_EXCEPTION
	if (ex && !boost::get_error_info<ContextTraceErrorInfo>(*ex))
		*ex << ContextTraceErrorInfo(context);
#endif /* NO_CAST_EXCEPTION */

	real_cxa_throw(obj, tinfo, dest);
}
#endif /* HAVE_CXXABI_H */

StackTrace *icinga::GetLastExceptionStack()
{
	return l_LastExceptionStack.get();
}

void icinga::SetLastExceptionStack(const StackTrace& trace)
{
	l_LastExceptionStack.reset(new StackTrace(trace));
}

ContextTrace *icinga::GetLastExceptionContext()
{
	return l_LastExceptionContext.get();
}

void icinga::SetLastExceptionContext(const ContextTrace& context)
{
	l_LastExceptionContext.reset(new ContextTrace(context));
}

String icinga::DiagnosticInformation(const std::exception& ex, bool verbose, StackTrace *stack, ContextTrace *context)
{
	std::ostringstream result;

	String message = ex.what();

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
		const StackTrace *st = boost::get_error_info<StackTraceErrorInfo>(ex);

		if (st) {
			result << *st;
		} else {
			result << std::endl;

			if (!stack)
				stack = GetLastExceptionStack();

			if (stack)
				result << *stack;

		}
	}

	const ContextTrace *ct = boost::get_error_info<ContextTraceErrorInfo>(ex);

	if (ct) {
		result << *ct;
	} else {
		result << std::endl;

		if (!context)
			context = GetLastExceptionContext();

		if (context)
			result << *context;
	}

	return result.str();
}

String icinga::DiagnosticInformation(const boost::exception_ptr& eptr, bool verbose)
{
	StackTrace *pt = GetLastExceptionStack();
	StackTrace stack;

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

ScriptError::~ScriptError() throw()
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
	return m_IncompleteExpr;;
}

bool ScriptError::IsHandledByDebugger() const
{
	return m_HandledByDebugger;
}

void ScriptError::SetHandledByDebugger(bool handled)
{
	m_HandledByDebugger = handled;
}

posix_error::posix_error()
	: m_Message(nullptr)
{ }

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

