/* Icinga 2 | (c) 2024 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/atomic.hpp"
#include <chrono>

namespace icinga
{

/**
 * Benchmark result.
 *
 * @ingroup base
 */
class Benchmark
{
public:
	using Clock = std::chrono::steady_clock;

	Benchmark& operator+=(const Clock::duration&) noexcept;
	Benchmark& operator+=(const Clock::time_point&) noexcept;

	/**
	 * @return The total accumulated time in seconds
	 */
	template<class T>
	explicit operator T() const noexcept
	{
		return std::chrono::duration<T>(Clock::duration(m_Sum.load(std::memory_order_relaxed))).count();
	}

private:
	Atomic<Clock::duration::rep> m_Sum {0};
};

}
