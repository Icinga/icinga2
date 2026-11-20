// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "icinga/host.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_configobject)

BOOST_AUTO_TEST_CASE(restore_unmodified_attribute_keeps_value)
{
	Host::Ptr host = new Host();
	BOOST_REQUIRE(host->GetEnableActiveChecks());

	host->RestoreAttribute("enable_active_checks");

	BOOST_CHECK(host->GetEnableActiveChecks());
	BOOST_CHECK(!host->IsAttributeModified("enable_active_checks"));
}

BOOST_AUTO_TEST_CASE(restore_modified_attribute_returns_original)
{
	Host::Ptr host = new Host();

	host->ModifyAttribute("enable_active_checks", false);
	BOOST_REQUIRE(!host->GetEnableActiveChecks());
	BOOST_REQUIRE(host->IsAttributeModified("enable_active_checks"));

	host->RestoreAttribute("enable_active_checks");

	BOOST_CHECK(host->GetEnableActiveChecks());
	BOOST_CHECK(!host->IsAttributeModified("enable_active_checks"));
}

BOOST_AUTO_TEST_SUITE_END()
