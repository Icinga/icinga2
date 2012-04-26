#include "i2-base.h"

using namespace icinga;

Exception::Exception(void)
{
}

Exception::Exception(const string& message)
{
	SetMessage(message);
}

string Exception::GetMessage(void) const
{
	return m_Message;
}

void Exception::SetMessage(string message)
{
	m_Message = message;
}

#ifdef _WIN32
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

string PosixException::FormatErrorCode(int code)
{
	return strerror(code);
}

string OpenSSLException::FormatErrorCode(int code)
{
	const char *message = ERR_error_string(code, NULL);

	if (message == NULL)
		message = "Unknown error.";

	return message;
}
