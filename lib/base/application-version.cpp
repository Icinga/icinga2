/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/application.hpp"
#include "icinga-version.h"
#include "icinga-spec-version.h"

using namespace icinga;

String Application::GetAppVersion()
{
	return VERSION;
}

String Application::GetAppSpecVersion()
{
	return SPEC_VERSION;
}
