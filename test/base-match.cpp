/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/utility.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_match)

BOOST_AUTO_TEST_CASE(tolong)
{
	BOOST_CHECK(Utility::Match("*", "hello"));
	BOOST_CHECK(!Utility::Match("\\**", "hello"));
	BOOST_CHECK(Utility::Match("\\**", "*ello"));
	BOOST_CHECK(Utility::Match("?e*l?", "hello"));
	BOOST_CHECK(Utility::Match("?e*l?", "helo"));
	BOOST_CHECK(!Utility::Match("world", "hello"));
	BOOST_CHECK(!Utility::Match("hee*", "hello"));
	BOOST_CHECK(Utility::Match("he??o", "hello"));
	BOOST_CHECK(Utility::Match("he?", "hel"));
	BOOST_CHECK(Utility::Match("he*", "hello"));
	BOOST_CHECK(Utility::Match("he*o", "heo"));
	BOOST_CHECK(Utility::Match("he**o", "heo"));
	BOOST_CHECK(Utility::Match("he**o", "hello"));
}

BOOST_AUTO_TEST_SUITE_END()
