/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef ATOMIC_H
#define ATOMIC_H

#include <atomic>

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

}

#endif /* ATOMIC_H */
