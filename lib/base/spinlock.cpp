/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#include "base/spinlock.hpp"
#include <atomic>

using namespace icinga;

void SpinLock::lock()
{
	while (m_Locked.test_and_set(std::memory_order_acquire)) {
	}
}

bool SpinLock::try_lock()
{
	return !m_Locked.test_and_set(std::memory_order_acquire);
}

void SpinLock::unlock()
{
	m_Locked.clear(std::memory_order_release);
}
