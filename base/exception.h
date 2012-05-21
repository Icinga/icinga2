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
class I2_BASE_API Exception : exception
{
private:
	char *m_Message;

protected:
	void SetMessage(const char *message);

public:
	Exception(void);
	Exception(const char *message);
	virtual ~Exception(void) throw();

	const char *GetMessage(void) const;

	virtual const char *what(void) const throw();
};

#define DEFINE_EXCEPTION_CLASS(klass)					\
	class klass : public Exception					\
	{								\
	public:								\
		inline klass(void) : Exception()			\
		{							\
		}							\
									\
		inline klass(const char *message)			\
		    : Exception(message)				\
		{							\
		}							\
	}

/**
 * An exception that is thrown when a certain feature
 * is not implemented.
 *
 * @ingroup base
 */
DEFINE_EXCEPTION_CLASS(NotImplementedException);

/**
 * An exception that is thrown when an argument to
 * a function is invalid.
 *
 * @ingroup base
 */
DEFINE_EXCEPTION_CLASS(InvalidArgumentException);

/**
 * An exception that is thrown when a cast yields
 * an invalid result.
 *
 * @ingroup base
 */
DEFINE_EXCEPTION_CLASS(InvalidCastException);

#ifdef _WIN32
/**
 * A Win32 error encapsulated in an exception.
 */
class Win32Exception : public Exception
{
public:
	/**
	 * Constructor for the Win32Exception class.
	 *
	 * @param message An error message.
	 * @param errorCode A Win32 error code.
	 */
	inline Win32Exception(const string& message, int errorCode)
	{
		string msg = message + ": " + FormatErrorCode(errorCode);
		SetMessage(msg.c_str());
	}

	/**
	 * Returns a string that describes the Win32 error.
	 *
	 * @param code The Win32 error code.
	 * @returns A description of the error.
	 */
	static string FormatErrorCode(int code);
};
#endif /* _WIN32 */

/**
 * A Posix error encapsulated in an exception.
 */
class PosixException : public Exception
{
public:
	/**
	 * Constructor for the PosixException class.
	 *
	 * @param message An error message.
	 * @param errorCode A Posix (errno) error code.
	 */
	inline PosixException(const string& message, int errorCode)
	{
		string msg = message + ": " + FormatErrorCode(errorCode);
		SetMessage(msg.c_str());
	}

	/**
	 * Returns a string that describes the Posix error.
	 *
	 * @param code The Posix error code.
	 * @returns A description of the error.
	 */
	static string FormatErrorCode(int code);
};

/**
 * An OpenSSL error encapsulated in an exception.
 */
class OpenSSLException : public Exception
{
public:
	/**
	 * Constructor for the OpenSSLException class.
	 *
	 * @param message An error message.
	 * @param errorCode An OpenSSL error code.
	 */
	inline OpenSSLException(const string& message, int errorCode)
	{
		string msg = message + ": " + FormatErrorCode(errorCode);
		SetMessage(msg.c_str());
	}

	/**
	 * Returns a string that describes the OpenSSL error.
	 *
	 * @param code The OpenSSL error code.
	 * @returns A description of the error.
	 */
	static string FormatErrorCode(int code);
};

}

#endif /* EXCEPTION_H */
