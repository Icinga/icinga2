/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef SHARED_H
#define SHARED_H

#include "base/atomic.hpp"
#include <boost/smart_ptr/intrusive_ptr.hpp>
#include <cstdint>
#include <utility>

namespace icinga
{

template<class T>
class Shared;

template<class T>
inline void intrusive_ptr_add_ref(Shared<T> *object)
{
	object->m_References.fetch_add(1);
}

template<class T>
inline void intrusive_ptr_release(Shared<T> *object)
{
	if (object->m_References.fetch_sub(1) == 1u) {
		delete object;
	}
}

/**
 * Seamless wrapper for any class to create shared pointers of.
 * Saves a memory allocation compared to std::shared_ptr.
 *
 * @ingroup base
 */
template<class T>
class Shared : public T
{
	friend void intrusive_ptr_add_ref<>(Shared<T> *object);
	friend void intrusive_ptr_release<>(Shared<T> *object);

public:
	typedef boost::intrusive_ptr<Shared> Ptr;

	/**
	 * Like std::make_shared, but for this class.
	 *
	 * @param args Constructor arguments
	 *
	 * @return Ptr
	 */
	template<class... Args>
	static inline
	Ptr Make(Args&&... args)
	{
		return new Shared(std::forward<Args>(args)...);
	}

	inline Shared(const Shared& origin) : Shared((const T&)origin)
	{
	}

	inline Shared(Shared&& origin) : Shared((T&&)origin)
	{
	}

	template<class... Args>
	inline Shared(Args&&... args) : T(std::forward<Args>(args)...), m_References(0)
	{
	}

	inline Shared& operator=(const Shared& rhs)
	{
		return operator=((const T&)rhs);
	}

	inline Shared& operator=(Shared&& rhs)
	{
		return operator=((T&&)rhs);
	}

	inline Shared& operator=(const T& rhs)
	{
		T::operator=(rhs);
		return *this;
	}

	inline Shared& operator=(T&& rhs)
	{
		T::operator=(std::move(rhs));
		return *this;
	}

private:
	Atomic<uint_fast64_t> m_References;
};

}

#endif /* SHARED_H */
