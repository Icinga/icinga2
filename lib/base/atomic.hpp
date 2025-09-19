/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef ATOMIC_H
#define ATOMIC_H

#include <atomic>
#include <mutex>

namespace icinga
{

/**
 * Like std::atomic, but enforces usage of its only safe constructor.
 *
 * "The default-initialized std::atomic<T> does not contain a T object,
 * and its only valid uses are destruction and
 * initialization by std::atomic_init, see LWG issue 2334."
 *  -- https://en.cppreference.com/w/cpp/atomic/atomic/atomic
 *
 * @ingroup base
 */
template<class T>
class Atomic : public std::atomic<T> {
public:
	/**
	 * The only safe constructor of std::atomic#atomic
	 *
	 * @param desired Initial value
	 */
	inline Atomic(T desired) : std::atomic<T>(desired)
	{
	}
};

class LockedMutex;

/**
 * Wraps any T into an interface similar to std::atomic<T>, that locks using a mutex.
 *
 * In contrast to std::atomic<T>, Locked<T> is also valid for types that are not trivially copyable.
 * In case T is trivially copyable, std::atomic<T> is almost certainly the better choice.
 *
 * @ingroup base
 */
template<typename T>
class Locked
{
public:
	T load(LockedMutex& mtx) const;
	void store(T desired, LockedMutex& mtx);

private:
	T m_Value;
};

/**
 * Wraps std::mutex, so that only Locked<T> can (un)lock it.
 *
 * The latter tiny lock scope is enforced this way to prevent deadlocks while passing around mutexes.
 *
 * @ingroup base
 */
class LockedMutex
{
	template<class T>
	friend class Locked;

private:
	std::mutex m_Mutex;
};

template<class T>
T Locked<T>::load(LockedMutex& mtx) const
{
	std::unique_lock lock (mtx.m_Mutex);

	return m_Value;
}

template<class T>
void Locked<T>::store(T desired, LockedMutex& mtx)
{
	std::unique_lock lock (mtx.m_Mutex);

	m_Value = std::move(desired);
}

}

#endif /* ATOMIC_H */
