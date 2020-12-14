/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "base/atomic.hpp"
#include <chrono>
#include <ratio>

namespace icinga
{

class BenchmarkSummary;

/**
 * Stopwatch for benchmarking.
 *
 * @ingroup base
 */
class BenchmarkStopWatch {
public:
	void Start()
	{
		m_Start = std::chrono::steady_clock::now();
	}

	void Stop(BenchmarkSummary& summary);

private:
	decltype(std::chrono::steady_clock::now()) m_Start;
};

/**
 * Benchmark result.
 *
 * @ingroup base
 */
class BenchmarkSummary
{
	friend BenchmarkStopWatch;

public:
	inline BenchmarkSummary() : m_Sum(0)
	{
	}

	inline long double GetSeconds() const
	{
		typedef std::ratio_divide<std::chrono::steady_clock::duration::period, std::chrono::seconds::period> factor;
		return (long double)m_Sum.load() * factor::num / factor::den;
	}

private:
	Atomic<std::chrono::steady_clock::duration::rep> m_Sum;
};

}

#endif /* BENCHMARK_H */
