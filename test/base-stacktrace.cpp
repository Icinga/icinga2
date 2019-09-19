/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/stacktrace.hpp"
#include <BoostTestTargetConfig.h>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_stacktrace)

BOOST_AUTO_TEST_CASE(stacktrace)
{
	StackTrace st;
	std::ostringstream obuf;
	obuf << st;
	BOOST_CHECK(obuf.str().size() > 0);
}

BOOST_AUTO_TEST_SUITE_END()
