/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
