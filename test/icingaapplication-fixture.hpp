// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef ICINGAAPPLICATION_FIXTURE_H
#define ICINGAAPPLICATION_FIXTURE_H

#include "icinga/icingaapplication.hpp"
#include "base/application.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

struct IcingaApplicationFixture
{
	IcingaApplicationFixture();

	void InitIcingaApplication();

	~IcingaApplicationFixture();
};

#endif // ICINGAAPPLICATION_FIXTURE_H
