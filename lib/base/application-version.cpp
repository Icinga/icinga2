// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
