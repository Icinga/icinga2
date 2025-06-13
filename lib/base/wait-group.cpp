/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#include "base/wait-group.hpp"

using namespace icinga;

bool StoppableWaitGroup::try_lock_shared()
{
	std::unique_lock lock (m_Mutex);

	if (m_Stopped) {
		return false;
	}

	++m_SharedLocks;
	return true;
}

void StoppableWaitGroup::unlock_shared()
{
	std::unique_lock lock (m_Mutex);

	if (!--m_SharedLocks && m_Stopped) {
		lock.unlock();
		m_CV.notify_all();
	}
}

bool StoppableWaitGroup::IsLockable() const
{
	return !m_Stopped.load(std::memory_order_relaxed);
}

/**
 * Disallow new shared locks, wait for all existing ones.
 */
void StoppableWaitGroup::Join()
{
	std::unique_lock lock (m_Mutex);

	m_Stopped.store(true, std::memory_order_relaxed);
	m_CV.wait(lock, [this] { return !m_SharedLocks; });
}
