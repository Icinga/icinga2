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
 * Exception
 *
 * Base class for all exceptions.
 */
class I2_BASE_API Exception
{
private:
	string m_Message;

protected:
	void SetMessage(string message);

public:
	Exception(void);
	Exception(const string& message);

	/**
	 * Destructor for the Exception class. Required for RTTI.
	 */
	virtual ~Exception(void)
	{
	}

	string GetMessage(void) const;
};

#define DEFINE_EXCEPTION_CLASS(klass)					\
	class klass : public Exception					\
	{								\
	public:								\
		inline klass(void) : Exception()			\
		{							\
		}							\
									\
		inline klass(const string& message)			\
		    : Exception(message)				\
		{							\
		}							\
	}

DEFINE_EXCEPTION_CLASS(NotImplementedException);
DEFINE_EXCEPTION_CLASS(InvalidArgumentException);

#ifdef _WIN32
class Win32Exception : public Exception
{
public:
	inline Win32Exception(const string& message, int errorCode)
	{
		SetMessage(message + ": " + FormatErrorCode(errorCode));
	}

	static string FormatErrorCode(int code);
};
#endif /* _WIN32 */

class PosixException : public Exception
{
public:
	inline PosixException(const string& message, int errorCode)
	{
		SetMessage(message + ": " + FormatErrorCode(errorCode));
	}

	static string FormatErrorCode(int code);
};

class OpenSSLException : public Exception
{
public:
	inline OpenSSLException(const string& message, int errorCode)
	{
		SetMessage(message + ": " + FormatErrorCode(errorCode));
	}

	static string FormatErrorCode(int code);
};

}

#endif /* EXCEPTION_H */
