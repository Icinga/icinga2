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
	 * ~Exception
	 *
	 * Required for RTTI.
	 */
	virtual ~Exception(void)
	{
	}

	string GetMessage(void) const;
};

#define DEFINE_EXCEPTION_CLASS(klass)								\
	class klass : public Exception									\
	{																\
	public:															\
		inline klass(void) : Exception()							\
		{															\
		}															\
																	\
		inline klass(const string& message) : Exception(message)	\
		{															\
		}															\
	};

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
