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

#include "base/exception.hpp"
#include <boost/thread/tss.hpp>

#ifdef HAVE_CXXABI_H
#	include <cxxabi.h>
#endif /* HAVE_CXXABI_H */

using namespace icinga;

static boost::thread_specific_ptr<StackTrace> l_LastExceptionStack;
static boost::thread_specific_ptr<ContextTrace> l_LastExceptionContext;

#ifdef HAVE_CXXABI_H
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

extern "C" void __cxa_throw(void *obj, TYPEINFO_TYPE *pvtinfo, void (*dest)(void *));
extern "C" void __cxa_rethrow_primary_exception(void* thrown_object);
#	endif /* !__GLIBCXX__ && !_WIN32 */
#endif /* HAVE_CXXABI_H */

void icinga::RethrowUncaughtException(void)
{
#if defined(__GLIBCXX__) || !defined(HAVE_CXXABI_H)
	throw;
#else /* __GLIBCXX__ || _WIN32 */
	__cxa_throw(*l_LastExceptionObj.get(), *l_LastExceptionPvtInfo.get(), *l_LastExceptionDest.get());
#endif /* __GLIBCXX__ || _WIN32 */
}

#ifdef HAVE_CXXABI_H
extern "C"
void __cxa_throw(void *obj, TYPEINFO_TYPE *pvtinfo, void (*dest)(void *))
{
	std::type_info *tinfo = static_cast<std::type_info *>(pvtinfo);

	typedef void (*cxa_throw_fn)(void *, std::type_info *, void (*)(void *)) __attribute__((noreturn));
	static cxa_throw_fn real_cxa_throw;

#ifndef __GLIBCXX__
	l_LastExceptionObj.reset(new void *(obj));
	l_LastExceptionPvtInfo.reset(new TYPEINFO_TYPE *(pvtinfo));
	l_LastExceptionDest.reset(new DestCallback(dest));
#endif /* __GLIBCXX__ */

	if (real_cxa_throw == 0)
		real_cxa_throw = (cxa_throw_fn)dlsym(RTLD_NEXT, "__cxa_throw");

#ifdef __GLIBCXX__
	void *thrown_ptr = obj;
	const std::type_info *boost_exc = &typeid(boost::exception);
	const std::type_info *user_exc = &typeid(user_error);

	/* Check if the exception is a pointer type. */
	if (tinfo->__is_pointer_p())
		thrown_ptr = *(void **)thrown_ptr;

	if (!user_exc->__do_catch(tinfo, &thrown_ptr, 1)) {
#endif /* __GLIBCXX__ */
		StackTrace stack;
		SetLastExceptionStack(stack);

		ContextTrace context;
		SetLastExceptionContext(context);

#ifdef __GLIBCXX__
		/* Check if thrown_ptr inherits from boost::exception. */
		if (boost_exc->__do_catch(tinfo, &thrown_ptr, 1)) {
			boost::exception *ex = (boost::exception *)thrown_ptr;

			if (boost::get_error_info<StackTraceErrorInfo>(*ex) == NULL)
				*ex << StackTraceErrorInfo(stack);

			if (boost::get_error_info<ContextTraceErrorInfo>(*ex) == NULL)
				*ex << ContextTraceErrorInfo(context);
		}
	}
#endif /* __GLIBCXX__ */

	real_cxa_throw(obj, tinfo, dest);
}
#endif /* HAVE_CXXABI_H */

StackTrace *icinga::GetLastExceptionStack(void)
{
	return l_LastExceptionStack.get();
}

void icinga::SetLastExceptionStack(const StackTrace& trace)
{
	l_LastExceptionStack.reset(new StackTrace(trace));
}

ContextTrace *icinga::GetLastExceptionContext(void)
{
	return l_LastExceptionContext.get();
}

void icinga::SetLastExceptionContext(const ContextTrace& context)
{
	l_LastExceptionContext.reset(new ContextTrace(context));
}

String icinga::DiagnosticInformation(boost::exception_ptr eptr)
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
		return DiagnosticInformation(ex, pt ? &stack : NULL, pc ? &context : NULL);
	}

	return boost::diagnostic_information(eptr);
}

