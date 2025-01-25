/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#include "base/utility.hpp"
#include "icinga/downtime.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

static void CanBeTriggeredHelper(
	bool active, bool fixed, double relStart, double relEnd, double duration, Value relTriggered, Value relRemoved, bool canBeTriggered
)
{
	Downtime::Ptr dt = new Downtime();
	auto now (Utility::GetTime());

	dt->SetActive(active, true);
	dt->SetFixed(fixed, true);
	dt->SetStartTime(now + relStart, true);
	dt->SetEndTime(now + relEnd, true);
	dt->SetDuration(duration, true);

	if (!relTriggered.IsEmpty()) {
		dt->SetTriggerTime(now + relTriggered, true);
	}

	if (!relRemoved.IsEmpty()) {
		dt->SetRemoveTime(now + relRemoved, true);
	}

	BOOST_CHECK(dt->CanBeTriggered() == canBeTriggered);
}

BOOST_AUTO_TEST_SUITE(icinga_downtime)

BOOST_AUTO_TEST_CASE(canbetriggered_fixed)
{
	CanBeTriggeredHelper(true, true, -2, 8, 0, Empty, Empty, true);
}

BOOST_AUTO_TEST_CASE(canbetriggered_flexible)
{
	CanBeTriggeredHelper(true, false, -2, 8, 20, Empty, Empty, true);
}

BOOST_AUTO_TEST_CASE(canbetriggered_inactive)
{
	CanBeTriggeredHelper(false, true, -2, 8, 0, Empty, Empty, false);
}

BOOST_AUTO_TEST_CASE(canbetriggered_removed)
{
	CanBeTriggeredHelper(true, true, -2, 8, 0, Empty, -4, false);
}

BOOST_AUTO_TEST_CASE(canbetriggered_triggered)
{
	CanBeTriggeredHelper(true, true, -2, 8, 0, -1, Empty, false);
}

BOOST_AUTO_TEST_CASE(canbetriggered_expired)
{
	CanBeTriggeredHelper(true, true, -12, -2, 0, Empty, Empty, false);
}

BOOST_AUTO_TEST_CASE(canbetriggered_tooearly)
{
	CanBeTriggeredHelper(true, true, 2, 12, 0, Empty, Empty, false);
}

BOOST_AUTO_TEST_SUITE_END()
