#include "i2-base.h"

using namespace icinga;

/**
 * Exception
 *
 * Default constructor for the Exception class.
 */
Exception::Exception(void)
{
}

/**
 * Exception
 *
 * Constructor for the exception class.
 *
 * @param message A message describing the exception.
 */
Exception::Exception(const string& message)
{
	SetMessage(message);
}

/**
 * GetMessage
 *
 * Retrieves the description for the exception.
 *
 * @returns The description.
 */
string Exception::GetMessage(void) const
{
	return m_Message;
}

/**
 * SetMessage
 *
 * Sets the description for the exception.
 *
 * @param message The description.
 */
void Exception::SetMessage(string message)
{
	m_Message = message;
}

#ifdef _WIN32
/**
 * FormatError
 *
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
 * FormatError
 *
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
 * FormatError
 *
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
