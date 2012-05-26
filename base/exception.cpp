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

/**
 * Default constructor for the Exception class.
 */
Exception::Exception(void)
{
	m_Code = 0;
	m_Message = NULL;
}

/**
 * Constructor for the exception class.
 *
 * @param message A message describing the exception.
 */
Exception::Exception(const char *message)
{
	m_Code = 0;
	m_Message = NULL;
	SetMessage(message);
}

/**
 * Destructor for the Exception class. Must be virtual for RTTI to work.
 */
Exception::~Exception(void) throw()
{
	Memory::Free(m_Message);
}

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
const char *Exception::GetMessage(void) const
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
	return GetMessage();
}

/**
 * Sets the description for the exception.
 *
 * @param message The description.
 */
void Exception::SetMessage(const char *message)
{
	Memory::Free(m_Message);
	m_Message = Memory::StrDup(message);
}

#ifdef _WIN32
/**
 * Formats an Win32 error code.
 *
 * @param code The error code.
 * @returns A string describing the error.
 */
string Win32Exception::FormatErrorCode(int code)
{
	char *message;
	string result = "Unknown error.";

	DWORD rc = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM, NULL, code, 0, (char *)&message,
		0, NULL);

	if (rc != 0) {
		result = string(message);
		LocalFree(message);
	}

	return result;
}
#endif /* _WIN32 */

/**
 * Formats a Posix error code.
 *
 * @param code The error code.
 * @returns A string describing the error.
 */
string PosixException::FormatErrorCode(int code)
{
	return strerror(code);
}

/**
 * Formats an OpenSSL error code.
 *
 * @param code The error code.
 * @returns A string describing the error.
 */
string OpenSSLException::FormatErrorCode(int code)
{
	const char *message = ERR_error_string(code, NULL);

	if (message == NULL)
		message = "Unknown error.";

	return message;
}
