// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
