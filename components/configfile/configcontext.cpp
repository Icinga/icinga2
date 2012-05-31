#include <iostream>
#include "configcontext.h"

using namespace std;

ConfigContext::ConfigContext(istream *input)
{
	Input = input;
	InitializeScanner();
}

ConfigContext::~ConfigContext(void)
{
	DestroyScanner();
}

