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
 *  Also, the second template parameter allows to specify the default memory order once for all operations.
 *
 * @ingroup base
 */
template<class T, std::memory_order m = std::memory_order_seq_cst>
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

	// The following methods have an argument with a default of std::memory_order_seq_cst hardcoded in the base class.
	// Hence, we need to override them here to allow for a different default memory order.

	void store(T desired, std::memory_order mo = m) noexcept
	{
		std::atomic<T>::store(desired, mo);
	}

	T load(std::memory_order mo = m) const noexcept
	{
		return std::atomic<T>::load(mo);
	}

	T exchange(T desired, std::memory_order mo = m) noexcept
	{
		return std::atomic<T>::exchange(desired, mo);
	}

	bool compare_exchange_weak(T& expected, T desired, std::memory_order mo = m) noexcept
	{
		return std::atomic<T>::compare_exchange_weak(expected, desired, mo);
	}

	bool compare_exchange_strong(T& expected, T desired, std::memory_order mo = m) noexcept
	{
		return std::atomic<T>::compare_exchange_strong(expected, desired, mo);
	}

	T fetch_add(T delta, std::memory_order mo = m) noexcept
	{
		return std::atomic<T>::fetch_add(delta, mo);
	}

	T fetch_sub(T delta, std::memory_order mo = m) noexcept
	{
		return std::atomic<T>::fetch_sub(delta, mo);
	}

	T fetch_and(T mask, std::memory_order mo = m) noexcept
	{
		return std::atomic<T>::fetch_and(mask, mo);
	}

	T fetch_or(T mask, std::memory_order mo = m) noexcept
	{
		return std::atomic<T>::fetch_or(mask, mo);
	}

	T fetch_xor(T mask, std::memory_order mo = m) noexcept
	{
		return std::atomic<T>::fetch_xor(mask, mo);
	}

	// The following operators call non-virtual methods we have overridden above.
	// Hence, we need to override them here as well to allow for a different default memory order.

	T operator=(T desired) noexcept
	{
		store(desired);
		return desired;
	}

	operator T() const noexcept
	{
		return load();
	}

	T operator+=(T delta) noexcept
	{
		return fetch_add(delta) + delta;
	}

	T operator-=(T delta) noexcept
	{
		return fetch_sub(delta) - delta;
	}

	T operator++() noexcept
	{
		return *this += 1;
	}

	T operator++(int) noexcept
	{
		return fetch_add(1);
	}

	T operator--() noexcept
	{
		return *this -= 1;
	}

	T operator--(int) noexcept
	{
		return fetch_sub(1);
	}

	T operator&=(T mask) noexcept
	{
		return fetch_and(mask) & mask;
	}

	T operator|=(T mask) noexcept
	{
		return fetch_or(mask) | mask;
	}

	T operator^=(T mask) noexcept
	{
		return fetch_xor(mask) ^ mask;
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
