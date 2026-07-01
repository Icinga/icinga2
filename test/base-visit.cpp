// SPDX-FileCopyrightText: 2026 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include <BoostTestTargetConfig.h>
#include "base/visit.hpp"

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_visit)

// This test-case tests if Visit correctly dispatches on types that all provide the same interface.
BOOST_AUTO_TEST_CASE(same)
{
	using AllSameVariant = std::variant<std::string, std::vector<int>>;

	AllSameVariant testVar = std::string{"test"};
	BOOST_REQUIRE_EQUAL(Visit(testVar, [](auto& var) { return std::size(var); }), 4);

	testVar = std::vector{0, 1, 2, 3, 4};
	BOOST_REQUIRE_EQUAL(Visit(testVar, [](auto& var) { return std::size(var); }), 5);
}

// This test-case verifies that Visit dispatches different methods to the correct types.
// Note how when the `u` is removed from the `1234u` testVar assignment, the test will fail,
// because it defaults to the signed `int` variant member.
BOOST_AUTO_TEST_CASE(different)
{
	using NumbersVariant = std::variant<unsigned int, int, double>;

	auto testVisit = [](auto& var) {
		return Visit(
			var,
			[](unsigned int val) {
				BOOST_REQUIRE_EQUAL(val, 1234);
				return 1;
			},
			[](int val) {
				BOOST_REQUIRE_EQUAL(val, -15);
				return 2;
			},
			[](double val) {
				BOOST_REQUIRE_EQUAL(val, 74.1234);
				return 3;
			}
		);
	};

	NumbersVariant testVar = 1234u;
	BOOST_REQUIRE_EQUAL(testVisit(testVar), 1);

	testVar = -15;
	BOOST_REQUIRE_EQUAL(testVisit(testVar), 2);

	testVar = 74.1234;
	BOOST_REQUIRE_EQUAL(testVisit(testVar), 3);
}

BOOST_AUTO_TEST_SUITE_END()
