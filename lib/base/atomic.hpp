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
