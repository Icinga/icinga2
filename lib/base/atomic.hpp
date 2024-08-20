/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef ATOMIC_H
#define ATOMIC_H

#include <atomic>
#include <mutex>
#include <type_traits>
#include <utility>

namespace icinga
{

/**
 * Extends std::atomic with an atomic constructor.
 *
 * @ingroup base
 */
template<class T>
class Atomic : public std::atomic<T> {
public:
	/**
	 * Like std::atomic#atomic, but operates atomically
	 *
	 * @param desired Initial value
	 */
	inline Atomic(T desired)
	{
		this->store(desired);
	}

	/**
	 * Like std::atomic#atomic, but operates atomically
	 *
	 * @param desired Initial value
	 * @param order Initial store operation's memory order
	 */
	inline Atomic(T desired, std::memory_order order)
	{
		this->store(desired, order);
	}
};

class LockedMutex;

/**
 * Wraps any T into a std::atomic<T>-like interface that locks using a mutex.
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

/**
 * Extends std::atomic with a Locked-like interface.
 *
 * @ingroup base
 */
template<class T>
class AtomicPseudoLocked : public std::atomic<T> {
public:
	using std::atomic<T>::atomic;
	using std::atomic<T>::load;
	using std::atomic<T>::store;

	T load(LockedMutex&) const
	{
		return load();
	}

	void store(T desired, LockedMutex&)
	{
		store(desired);
	}
};

/**
 * Type alias for AtomicPseudoLocked<T> if possible, otherwise Locked<T> is used as a fallback.
 *
 * @ingroup base
 */
template <typename T>
using AtomicOrLocked =
#if defined(__GNUC__) && __GNUC__ < 5
	// GCC does not implement std::is_trivially_copyable until version 5.
	typename std::conditional<std::is_fundamental<T>::value || std::is_pointer<T>::value, AtomicPseudoLocked<T>, Locked<T>>::type;
#else /* defined(__GNUC__) && __GNUC__ < 5 */
	typename std::conditional<std::is_trivially_copyable<T>::value, AtomicPseudoLocked<T>, Locked<T>>::type;
#endif /* defined(__GNUC__) && __GNUC__ < 5 */

}

#endif /* ATOMIC_H */
