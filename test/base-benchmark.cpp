/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/benchmark.hpp"
#include "base/utility.hpp"
#include <BoostTestTargetConfig.h>
#include <cmath>

using namespace icinga;

static bool AssertSumSeconds(BenchmarkSummary& sum, long double seconds)
{
	return std::abs((sum.GetSeconds() - seconds) / seconds) < 0.05;
}

BOOST_AUTO_TEST_SUITE(base_benchmark)

BOOST_AUTO_TEST_CASE(one_second)
{
	BenchmarkSummary sum;
	BenchmarkStopWatch sw;

	sw.Start();
	Utility::Sleep(1);
	sw.Stop(sum);

	BOOST_CHECK(AssertSumSeconds(sum, 1));
}

BOOST_AUTO_TEST_CASE(two_seconds)
{
	BenchmarkSummary sum;
	BenchmarkStopWatch sw;

	sw.Start();
	Utility::Sleep(1);
	sw.Stop(sum);

	sw.Start();
	Utility::Sleep(1);
	sw.Stop(sum);

	BOOST_CHECK(AssertSumSeconds(sum, 2));
}

BOOST_AUTO_TEST_SUITE_END()
