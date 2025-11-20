/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/application.hpp"
#include "config.h"

using namespace icinga;

String Application::GetAppVersion()
{
	return ICINGA_VERSION;
}

bool Application::GetAppIncludePackageInfo()
{
	return ICINGA2_INCLUDE_PACKAGE_INFO;
}

String Application::GetAppPackageVersion()
{
	return ICINGA_PACKAGE_VERSION;
}

String Application::GetAppPackageRevision()
{
	return ICINGA_PACKAGE_REVISION;
}

String Application::GetAppPackageVendor()
{
	return ICINGA_PACKAGE_VENDOR;
}
