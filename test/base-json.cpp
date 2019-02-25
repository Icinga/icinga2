/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/dictionary.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_json)

BOOST_AUTO_TEST_CASE(invalid1)
{
	BOOST_CHECK_THROW(JsonDecode("\"1.7"), std::exception);
	BOOST_CHECK_THROW(JsonDecode("{8: \"test\"}"), std::exception);
	BOOST_CHECK_THROW(JsonDecode("{\"test\": \"test\""), std::exception);
}

BOOST_AUTO_TEST_SUITE_END()
