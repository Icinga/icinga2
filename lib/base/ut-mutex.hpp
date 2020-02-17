/* Icinga 2 | (c) 2020 Icinga GmbH | GPLv2+ */

#ifndef UT_MUTEX_H
#define UT_MUTEX_H

#include <atomic>
#include <cstdint>
#include <thread>
#include "base/ut-id.hpp"

namespace icinga
{
namespace UT
{
namespace Aware
{

/**
 * Like std::mutex, but UserspaceThread-aware.
 *
 * @ingroup base
 */
class Mutex
{
public:
	inline Mutex() = default;

	Mutex(const Mutex&) = delete;
	Mutex(Mutex&&) = delete;
	Mutex& operator=(const Mutex&) = delete;
	Mutex& operator=(Mutex&&) = delete;

	void lock();

	inline bool try_lock()
	{
		return !m_Locked.test_and_set(std::memory_order_acquire);
	}

	inline void unlock()
	{
		m_Locked.clear(std::memory_order_release);
	}

private:
	std::atomic_flag m_Locked = ATOMIC_FLAG_INIT;
};

/**
 * Like std::recursive_mutex, but UserspaceThread-aware.
 *
 * @ingroup base
 */
class RecursiveMutex
{
public:
	RecursiveMutex();

	RecursiveMutex(const RecursiveMutex&) = delete;
	RecursiveMutex(RecursiveMutex&&) = delete;
	RecursiveMutex& operator=(const RecursiveMutex&) = delete;
	RecursiveMutex& operator=(RecursiveMutex&&) = delete;

	void lock();
	bool try_lock();
	void unlock();

private:
	std::atomic<std::thread::id> m_KernelspaceOwner;
	std::atomic<ID> m_UserspaceOwner;
	uint_fast32_t m_Depth;
	Mutex m_Mutex;
};

}
}
}

#endif /* UT_MUTEX_H */
