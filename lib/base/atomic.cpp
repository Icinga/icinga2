/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "base/atomic.hpp"

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
AtomicDuration& AtomicDuration::operator+=(const Clock::duration& elapsedTime) noexcept
{
	m_Sum.fetch_add(elapsedTime.count(), std::memory_order_relaxed);
	return *this;
}
