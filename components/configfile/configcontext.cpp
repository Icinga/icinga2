#include "i2-configfile.h"

using namespace icinga;

ConfigContext::ConfigContext(istream *input)
{
	Input = input;
	InitializeScanner();
}

ConfigContext::~ConfigContext(void)
{
	DestroyScanner();
}

