/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#include "base/benchmark.hpp"
#include <BoostTestTargetConfig.h>
#include <chrono>

using namespace icinga;

BOOST_AUTO_TEST_SUITE(base_benchmark)

BOOST_AUTO_TEST_CASE(zero)
{
	BOOST_CHECK_EQUAL((double)Benchmark(), 0);
}

BOOST_AUTO_TEST_CASE(one)
{
	Benchmark sum;

	sum += std::chrono::seconds(1);

	BOOST_CHECK_EQUAL((double)sum, 1);
}

BOOST_AUTO_TEST_CASE(two)
{
	Benchmark sum;

	sum += std::chrono::seconds(1);
	sum += std::chrono::seconds(2);

	BOOST_CHECK_EQUAL((double)sum, 3);
}

BOOST_AUTO_TEST_SUITE_END()
