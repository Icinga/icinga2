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

#ifndef EXCEPTION_H
#define EXCEPTION_H

namespace icinga
{

/**
 * Base class for all exceptions.
 *
 * @ingroup base
 */
class I2_BASE_API Exception : public virtual exception
{
public:
	Exception(void)
	    : m_Message(), m_Code(0)
	{ }

	Exception(String message)
	    : m_Message(message), m_Code(0)
	{ }

	Exception(String message, int code)
	    : m_Message(message), m_Code(code)
	{ }

	/**
	 * Destructor for the Exception class. Must be virtual for RTTI to work.
	 */
	virtual ~Exception(void) throw()
	{ }

	int GetCode(void) const;
	String GetMessage(void) const;

	virtual const char *what(void) const throw();

	static StackTrace *GetLastStackTrace(void);
	static void SetLastStackTrace(const StackTrace& trace);

protected:
	void SetCode(int code);
	void SetMessage(String message);

private:
	String m_Message;
	int m_Code;

	static StackTrace *m_StackTrace;
};

typedef boost::error_info<StackTrace, StackTrace> StackTraceErrorInfo;

#define DEFINE_EXCEPTION_CLASS(klass)					\
	class klass : public Exception					\
	{								\
	public:								\
		inline klass(void) : Exception()			\
		{ }							\
									\
		inline klass(String message)				\
		    : Exception(message)				\
		{ }							\
	}

/**
 * An exception that is thrown when a certain feature
 * is not implemented.
 *
 * @ingroup base
 */
DEFINE_EXCEPTION_CLASS(NotImplementedException);

#ifdef _WIN32
/**
 * A Win32 error encapsulated in an exception.
 *
 * @ingroup base
 */
class I2_BASE_API Win32Exception : public Exception
{
public:
	/**
	 * Constructor for the Win32Exception class.
	 *
	 * @param message An error message.
	 * @param errorCode A Win32 error code.
	 */
	inline Win32Exception(const String& message, int errorCode)
	    : Exception(message + ": " + FormatErrorCode(errorCode), errorCode)
	{ }

	/**
	 * Returns a String that describes the Win32 error.
	 *
	 * @param code The Win32 error code.
	 * @returns A description of the error.
	 */
	static String FormatErrorCode(int code);
};
#endif /* _WIN32 */

/**
 * A Posix error encapsulated in an exception.
 *
 * @ingroup base
 */
class I2_BASE_API PosixException : public Exception
{
public:
	/**
	 * Constructor for the PosixException class.
	 *
	 * @param message An error message.
	 * @param errorCode A Posix (errno) error code.
	 */
	inline PosixException(const String& message, int errorCode)
	    : Exception(message + ": " + FormatErrorCode(errorCode), errorCode)
	{ }

	/**
	 * Returns a String that describes the Posix error.
	 *
	 * @param code The Posix error code.
	 * @returns A description of the error.
	 */
	static String FormatErrorCode(int code);
};

/**
 * An OpenSSL error encapsulated in an exception.
 *
 * @ingroup base
 */
class I2_BASE_API OpenSSLException : public Exception
{
public:
	/**
	 * Constructor for the OpenSSLException class.
	 *
	 * @param message An error message.
	 * @param errorCode An OpenSSL error code.
	 */
	inline OpenSSLException(const String& message, int errorCode)
	    : Exception(message + ": " + FormatErrorCode(errorCode), errorCode)
	{ }

	/**
	 * Returns a String that describes the OpenSSL error.
	 *
	 * @param code The OpenSSL error code.
	 * @returns A description of the error.
	 */
	static String FormatErrorCode(int code);
};

}

#endif /* EXCEPTION_H */
