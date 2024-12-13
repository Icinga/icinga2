/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#include "base/benchmark.hpp"

using namespace icinga;

/**
 * Adds the elapsedTime to this instance.
 *
 * May be called multiple times to accumulate time.
 *
 * @param elapsedTime The distance between two time points
 *
 * @return This instance for method chaining
 */
Benchmark& Benchmark::operator+=(const Clock::duration& elapsedTime) noexcept
{
	m_Sum.fetch_add(elapsedTime.count(), std::memory_order_relaxed);
	return *this;
}

/**
 * Adds the time elapsed since startTime to this instance.
 *
 * May be called multiple times to accumulate time.
 *
 * @param startTime The start time to subtract from the current time
 *
 * @return This instance for method chaining
 */
Benchmark& Benchmark::operator+=(const Clock::time_point& startTime) noexcept
{
	return *this += Clock::now() - startTime;
}
