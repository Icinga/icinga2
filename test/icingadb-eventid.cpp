// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icingadb/icingadb.hpp"
#include "icinga/host.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_CASE(calc_event_id_uses_object_identifier)
{
	Host::Ptr host = new Host();
	host->SetName("master01");

	host->SetIcingadbIdentifier("id-a");
	auto first = IcingaDB::CalcEventID("state_change", host, 1710000000.123);

	host->SetIcingadbIdentifier("id-b");
	auto second = IcingaDB::CalcEventID("state_change", host, 1710000000.123);

	BOOST_CHECK_NE(first, second);
}
