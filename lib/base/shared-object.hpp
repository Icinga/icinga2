/* Icinga 2 | (c) 2019 Icinga GmbH | GPLv2+ */

#ifndef SHARED_OBJECT_H
#define SHARED_OBJECT_H

#include "base/atomic.hpp"
#include "base/object.hpp"
#include <cstdint>

namespace icinga
{

class SharedObject;

inline void intrusive_ptr_add_ref(SharedObject *object);
inline void intrusive_ptr_release(SharedObject *object);

/**
 * Seamless and polymorphistic base for any class to create shared pointers of.
 * Saves a memory allocation compared to std::shared_ptr.
 *
 * @ingroup base
 */
class SharedObject
{
	friend void intrusive_ptr_add_ref(SharedObject *object);
	friend void intrusive_ptr_release(SharedObject *object);

protected:
	inline SharedObject() : m_References(0)
	{
	}

	inline SharedObject(const SharedObject&) : SharedObject()
	{
	}

	inline SharedObject(SharedObject&&) : SharedObject()
	{
	}

	inline SharedObject& operator=(const SharedObject&)
	{
		return *this;
	}

	inline SharedObject& operator=(SharedObject&&)
	{
		return *this;
	}

	~SharedObject() = default;

private:
	Atomic<uint_fast64_t> m_References;
};

inline void intrusive_ptr_add_ref(SharedObject *object)
{
	object->m_References.fetch_add(1);
}

inline void intrusive_ptr_release(SharedObject *object)
{
	if (object->m_References.fetch_sub(1) == 1u) {
		delete object;
	}
}

}

#endif /* SHARED_OBJECT_H */
