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
	inline T load() const
	{
		std::unique_lock<std::mutex> lock(m_Mutex);

		return m_Value;
	}

	inline void store(T desired)
	{
		std::unique_lock<std::mutex> lock(m_Mutex);

		m_Value = std::move(desired);
	}

private:
	mutable std::mutex m_Mutex;
	T m_Value;
};

/**
 * Type alias for std::atomic<T> if possible, otherwise Locked<T> is used as a fallback.
 *
 * @ingroup base
 */
template <typename T>
using AtomicOrLocked =
#if defined(__GNUC__) && __GNUC__ < 5
	// GCC does not implement std::is_trivially_copyable until version 5.
	typename std::conditional<std::is_fundamental<T>::value || std::is_pointer<T>::value, std::atomic<T>, Locked<T>>::type;
#else /* defined(__GNUC__) && __GNUC__ < 5 */
	typename std::conditional<std::is_trivially_copyable<T>::value, std::atomic<T>, Locked<T>>::type;
#endif /* defined(__GNUC__) && __GNUC__ < 5 */

}

#endif /* ATOMIC_H */
