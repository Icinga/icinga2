/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef SPINLOCK_H
#define SPINLOCK_H

#include <atomic>

namespace icinga
{

/**
 * A spin lock.
 *
 * @ingroup base
 */
class SpinLock
{
public:
	SpinLock() = default;
	SpinLock(const SpinLock&) = delete;
	SpinLock& operator=(const SpinLock&) = delete;
	SpinLock(SpinLock&&) = delete;
	SpinLock& operator=(SpinLock&&) = delete;

	void lock();
	bool try_lock();
	void unlock();

private:
	std::atomic_flag m_Locked = ATOMIC_FLAG_INIT;
};

}

#endif /* SPINLOCK_H */
