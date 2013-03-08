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

#include "i2-base.h"

using namespace icinga;

boost::thread_specific_ptr<StackTrace> Exception::m_LastStackTrace;

/**
 * Retrieves the error code for the exception.
 *
 * @returns The error code.
 */
int Exception::GetCode(void) const
{
	return m_Code;
}

/**
 * Sets the error code for the exception.
 *
 * @param code The error code.
 */
void Exception::SetCode(int code)
{
	m_Code = code;
}

/**
 * Retrieves the description for the exception.
 *
 * @returns The description.
 */
String Exception::GetMessage(void) const
{
	return m_Message;
}

/**
 * Retrieves the description for the exception.
 *
 * @returns The description.
 */
const char *Exception::what(void) const throw()
{
	return m_Message.CStr();
}

/**
 * Sets the description for the exception.
 *
 * @param message The description.
 */
void Exception::SetMessage(String message)
{
	m_Message = message;
}

#ifdef _WIN32
/**
 * Formats an Win32 error code.
 *
 * @param code The error code.
 * @returns A String describing the error.
 */
String Win32Exception::FormatErrorCode(int code)
{
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

	return result;
}
#endif /* _WIN32 */

/**
 * Formats a Posix error code.
 *
 * @param code The error code.
 * @returns A String describing the error.
 */
String PosixException::FormatErrorCode(int code)
{
	return strerror(code);
}

/**
 * Formats an OpenSSL error code.
 *
 * @param code The error code.
 * @returns A String describing the error.
 */
String OpenSSLException::FormatErrorCode(int code)
{
	const char *message = ERR_error_string(code, NULL);

	if (message == NULL)
		message = "Unknown error.";

	return message;
}

#ifndef _WIN32
extern "C"
void __cxa_throw(void *obj, void *pvtinfo, void (*dest)(void *))
{
	typedef void (*cxa_throw_fn)(void *, void *, void (*) (void *)) __attribute__((noreturn));
	static cxa_throw_fn real_cxa_throw;

	if (real_cxa_throw == 0)
		real_cxa_throw = (cxa_throw_fn)dlsym(RTLD_NEXT, "__cxa_throw");

	void *thrown_ptr = obj;
	const type_info *tinfo = static_cast<type_info *>(pvtinfo);
	const type_info *boost_exc = &typeid(boost::exception);

	/* Check if the exception is a pointer type. */
	if (tinfo->__is_pointer_p())
		thrown_ptr = *(void **)thrown_ptr;

	StackTrace trace;
	Exception::SetLastStackTrace(trace);

	/* Check if thrown_ptr inherits from boost::exception. */
	if (boost_exc->__do_catch(tinfo, &thrown_ptr, 1)) {
		boost::exception *ex = (boost::exception *)thrown_ptr;

		*ex << StackTraceErrorInfo(trace);
	}

	real_cxa_throw(obj, pvtinfo, dest);
}
#endif /* _WIN32 */

StackTrace *Exception::GetLastStackTrace(void)
{
	return m_LastStackTrace.get();
}

void Exception::SetLastStackTrace(const StackTrace& trace)
{
	m_LastStackTrace.reset(new StackTrace(trace));
}

