#include "i2-dyn.h"

using namespace icinga;

ConfigContext::ConfigContext(istream *input)
{
	m_Input = input;
	InitializeScanner();
}

ConfigContext::~ConfigContext(void)
{
	DestroyScanner();
}

size_t ConfigContext::ReadInput(char *buffer, size_t max_size)
{
	m_Input->read(buffer, max_size);
	return m_Input->gcount();
}

void *ConfigContext::GetScanner(void) const
{
	return m_Scanner;
}

