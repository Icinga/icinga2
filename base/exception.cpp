#include "i2-base.h"

using namespace icinga;

Exception::Exception(void)
{
}

Exception::Exception(const string& message)
{
	m_Message = message;
}

Exception::~Exception(void)
{
}

string Exception::GetMessage(void) const
{
	return m_Message;
}
