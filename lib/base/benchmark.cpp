/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/benchmark.hpp"

using namespace icinga;

void BenchmarkStopWatch::Stop(BenchmarkSummary& summary)
{
	summary.m_Sum.fetch_add((std::chrono::steady_clock::now() - m_Start).count());
}
