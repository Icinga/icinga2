#include "i2-icinga.h"

using namespace icinga;

Endpoint::Endpoint(void)
{
	m_Connected = false;
}

void Endpoint::SetConnected(bool connected)
{
	m_Connected = connected;
}

bool Endpoint::GetConnected(void)
{
	return m_Connected;
}
