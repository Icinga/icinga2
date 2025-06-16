/* Icinga 2 | (c) 2025 Icinga GmbH | GPLv2+ */

#pragma once

#include "base/object.hpp"
#include "base/atomic.hpp"
#include <condition_variable>
#include <cstdint>
#include <mutex>

namespace icinga
{

/**
 * A synchronization interface that allows concurrent shared locking.
 *
 * @ingroup base
 */
class WaitGroup : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(WaitGroup);

	virtual bool try_lock_shared() = 0;
	virtual void unlock_shared() = 0;

	virtual bool IsLockable() const = 0;
};

/**
 * A thread-safe wait group that can be stopped to prevent further shared locking.
 *
 * @ingroup base
 */
class StoppableWaitGroup : public WaitGroup
{
public:
	DECLARE_PTR_TYPEDEFS(StoppableWaitGroup);

	StoppableWaitGroup() = default;
	StoppableWaitGroup(const StoppableWaitGroup&) = delete;
	StoppableWaitGroup(StoppableWaitGroup&&) = delete;
	StoppableWaitGroup& operator=(const StoppableWaitGroup&) = delete;
	StoppableWaitGroup& operator=(StoppableWaitGroup&&) = delete;

	bool try_lock_shared() override;
	void unlock_shared() override;

	bool IsLockable() const override;

	void Join();

private:
	std::mutex m_Mutex;
	std::condition_variable m_CV;
	uint_fast32_t m_SharedLocks = 0;
	Atomic<bool> m_Stopped = false;
};

}
