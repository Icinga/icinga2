/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef ATOMIC_H
#define ATOMIC_H

#include <atomic>
#include <chrono>
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
	using std::atomic<T>::operator=;

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
 * Accumulates time durations atomically.
 *
 * @ingroup base
 */
class AtomicDuration
{
public:
	using Clock = std::chrono::steady_clock;

	/**
	 * Adds the elapsedTime to this instance.
	 *
	 * May be called multiple times to accumulate time.
	 *
	 * @param elapsedTime The distance between two time points
	 *
	 * @return This instance for method chaining
	 */
	AtomicDuration& operator+=(const Clock::duration& elapsedTime) noexcept
	{
		m_Sum.fetch_add(elapsedTime.count(), std::memory_order_relaxed);
		return *this;
	}

	/**
	 * @return The total accumulated time in seconds
	 */
	operator double() const noexcept
	{
		return std::chrono::duration<double>(Clock::duration(m_Sum.load(std::memory_order_relaxed))).count();
	}

private:
	Atomic<Clock::duration::rep> m_Sum {0};
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
	Locked() = default;

	Locked(T desired) : m_Value(std::move(desired))
	{
	}

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
 * Type alias for Atomic<T> if possible, otherwise Locked<T> is used as a fallback.
 *
 * @ingroup base
 */
template <typename T>
using AtomicOrLocked = std::conditional_t<std::is_trivially_copyable_v<T>, Atomic<T>, Locked<T>>;

}

#endif /* ATOMIC_H */
