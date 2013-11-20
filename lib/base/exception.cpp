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

#include "base/exception.h"

using namespace icinga;

static boost::thread_specific_ptr<StackTrace> l_LastStackTrace;

#ifndef _WIN32
extern "C"
void __cxa_throw(void *obj, void *pvtinfo, void (*dest)(void *))
{
	typedef void (*cxa_throw_fn)(void *, void *, void (*) (void *)) __attribute__((noreturn));
	static cxa_throw_fn real_cxa_throw;

	if (real_cxa_throw == 0)
		real_cxa_throw = (cxa_throw_fn)dlsym(RTLD_NEXT, "__cxa_throw");

#ifndef __APPLE__
	void *thrown_ptr = obj;
	const std::type_info *tinfo = static_cast<std::type_info *>(pvtinfo);
	const std::type_info *boost_exc = &typeid(boost::exception);

	/* Check if the exception is a pointer type. */
	if (tinfo->__is_pointer_p())
		thrown_ptr = *(void **)thrown_ptr;
#endif /* __APPLE__ */

	StackTrace trace;
	SetLastExceptionStack(trace);

#ifndef __APPLE__
	/* Check if thrown_ptr inherits from boost::exception. */
	if (boost_exc->__do_catch(tinfo, &thrown_ptr, 1)) {
		boost::exception *ex = (boost::exception *)thrown_ptr;

		*ex << StackTraceErrorInfo(trace);
	}
#endif /* __APPLE__ */

	real_cxa_throw(obj, pvtinfo, dest);
}
#endif /* _WIN32 */

StackTrace *icinga::GetLastExceptionStack(void)
{
	return l_LastStackTrace.get();
}

void icinga::SetLastExceptionStack(const StackTrace& trace)
{
	l_LastStackTrace.reset(new StackTrace(trace));
}

String icinga::DiagnosticInformation(boost::exception_ptr eptr)
{
	StackTrace *pt = GetLastExceptionStack();
	StackTrace trace;

	if (pt)
		trace = *pt;

	try {
		boost::rethrow_exception(eptr);
	} catch (const std::exception& ex) {
		return DiagnosticInformation(ex, pt ? &trace : NULL);
	}

	return boost::diagnostic_information(eptr);
}

