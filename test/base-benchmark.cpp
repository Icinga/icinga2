/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#include "base/benchmark.hpp"
#include "base/utility.hpp"
#include <BoostTestTargetConfig.h>
#include <chrono>
#include <cmath>

using namespace icinga;

static bool AssertSumSeconds(Benchmark& sum, double seconds)
{
	return std::abs(((double)sum - seconds) / seconds) < 0.05;
}

BOOST_AUTO_TEST_SUITE(base_benchmark)

BOOST_AUTO_TEST_CASE(zero)
{
	BOOST_CHECK_EQUAL((double)Benchmark(), 0);
}

BOOST_AUTO_TEST_CASE(one)
{
	Benchmark sum;

	auto start (Benchmark::Clock::now());
	Utility::Sleep(0.25);
	sum += start;

	BOOST_CHECK(AssertSumSeconds(sum, 0.25));
}

BOOST_AUTO_TEST_CASE(two)
{
	Benchmark sum;

	{
		auto start (Benchmark::Clock::now());
		Utility::Sleep(0.25);
		sum += start;
	}

	auto start (Benchmark::Clock::now());
	Utility::Sleep(0.5);
	sum += start;

	BOOST_CHECK(AssertSumSeconds(sum, 0.75));
}

BOOST_AUTO_TEST_SUITE_END()
