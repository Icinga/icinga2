/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef ATOMIC_H
#define ATOMIC_H

#include <atomic>
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
 * Wraps T into a std::atomic<T>-like interface.
 *
 * @ingroup base
 */
template<class T>
class NotAtomic
{
public:
	inline T load() const
	{
		return m_Value;
	}

	inline void store(T desired)
	{
		m_Value = std::move(desired);
	}

	T m_Value;
};

/**
 * Tells whether to use std::atomic<T> or NotAtomic<T>.
 *
 * @ingroup base
 */
template<class T>
struct Atomicable
{
	// Doesn't work with too old compilers.
	//static constexpr bool value = std::is_trivially_copyable<T>::value && sizeof(T) <= sizeof(void*);
	static constexpr bool value = (std::is_fundamental<T>::value || std::is_pointer<T>::value) && sizeof(T) <= sizeof(void*);
};

/**
 * Uses either std::atomic<T> or NotAtomic<T> depending on atomicable.
 *
 * @ingroup base
 */
template<bool atomicable>
struct AtomicTemplate;

template<>
struct AtomicTemplate<false>
{
	template<class T>
	struct tmplt
	{
		typedef NotAtomic<T> type;
	};
};

template<>
struct AtomicTemplate<true>
{
	template<class T>
	struct tmplt
	{
		typedef std::atomic<T> type;
	};
};

/**
 * Uses either std::atomic<T> or NotAtomic<T> depending on T.
 *
 * @ingroup base
 */
template<class T>
struct EventuallyAtomic
{
	typedef typename AtomicTemplate<Atomicable<T>::value>::template tmplt<T>::type type;
};

}

#endif /* ATOMIC_H */
