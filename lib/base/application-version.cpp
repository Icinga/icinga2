/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/application.hpp"
#include "icinga-version.h"

using namespace icinga;

String Application::GetAppVersion()
{
	return VERSION;
}

bool Application::GetAppIncludePackageInfo()
{
	return ICINGA2_INCLUDE_PACKAGE_INFO;
}

String Application::GetAppPackageVersion()
{
	return PACKAGE_VERSION;
}

String Application::GetAppPackageRevision()
{
	return PACKAGE_REVISION;
}

String Application::GetAppPackageVendor()
{
	return PACKAGE_VENDOR;
}
