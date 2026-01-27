// SPDX-FileCopyrightText: 2025 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/atomic.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_atomic)

BOOST_AUTO_TEST_CASE(duration_none)
{
	BOOST_CHECK_EQUAL(static_cast<double>(AtomicDuration()), 0);
}

BOOST_AUTO_TEST_CASE(duration_one)
{
	AtomicDuration sum;

	sum += std::chrono::seconds(1);

	BOOST_CHECK_EQUAL(static_cast<double>(sum), 1);
}

BOOST_AUTO_TEST_CASE(duration_two)
{
	AtomicDuration sum;

	sum += std::chrono::seconds(1);
	sum += std::chrono::seconds(2);

	BOOST_CHECK_EQUAL(static_cast<double>(sum), 3);
}

BOOST_AUTO_TEST_SUITE_END()
