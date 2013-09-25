/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef OBJECT_H
#define OBJECT_H

#include "base/i2-base.h"
#include "base/debug.h"
#include <boost/thread/thread.hpp>

#ifndef _DEBUG
#include <boost/thread/mutex.hpp>
#else /* _DEBUG */
#include <boost/thread/recursive_mutex.hpp>
#endif /* _DEBUG */

#include <boost/smart_ptr/shared_ptr.hpp>
#include <boost/smart_ptr/weak_ptr.hpp>
#include <boost/smart_ptr/enable_shared_from_this.hpp>

using boost::shared_ptr;
using boost::weak_ptr;
using boost::dynamic_pointer_cast;
using boost::static_pointer_cast;

namespace icinga
{

class Value;

#define DECLARE_PTR_TYPEDEFS(klass) \
	typedef shared_ptr<klass> Ptr; \
	typedef weak_ptr<klass> WeakPtr

/**
 * Base class for all heap-allocated objects. At least one of its methods
 * has to be virtual for RTTI to work.
 *
 * @ingroup base
 */
class I2_BASE_API Object : public boost::enable_shared_from_this<Object>
{
public:
	DECLARE_PTR_TYPEDEFS(Object);

	Object(void);
	virtual ~Object(void);

	/**
	 * Holds a shared pointer and provides support for implicit upcasts.
	 *
	 * @ingroup base
	 */
	class SharedPtrHolder
	{
	public:
		/**
		 * Constructor for the SharedPtrHolder class.
		 *
		 * @param object The shared pointer that should be used to
		 *		 construct this shared pointer holder.
		 */
		explicit SharedPtrHolder(const Object::Ptr& object)
			: m_Object(object)
		{ }

		/**
		 * Retrieves a shared pointer for the object that is associated
		 * this holder instance.
		 *
		 * @returns A shared pointer.
		 */
		template<typename T>
		operator shared_ptr<T>(void) const
		{
#ifdef _DEBUG
			shared_ptr<T> other = dynamic_pointer_cast<T>(m_Object);
			ASSERT(other);
#else /* _DEBUG */
			shared_ptr<T> other = static_pointer_cast<T>(m_Object);
#endif /* _DEBUG */

			return other;
		}

		/**
		 * Retrieves a weak pointer for the object that is associated
		 * with this holder instance.
		 *
		 * @returns A weak pointer.
		 */
		template<typename T>
		operator weak_ptr<T>(void) const
		{
			return static_cast<shared_ptr<T> >(*this);
		}

		operator Value(void) const;

	private:
		Object::Ptr m_Object; /**< The object that belongs to this
					   holder instance */
	};

#ifdef _DEBUG
	bool OwnsLock(void) const;
#endif /* _DEBUG */

protected:
	SharedPtrHolder GetSelf(void);

private:
	Object(const Object& other);
	Object& operator=(const Object& rhs);

#ifndef _DEBUG
	typedef boost::mutex MutexType;
#else /* _DEBUG */
	typedef boost::recursive_mutex MutexType;

	static boost::mutex m_DebugMutex;
	mutable bool m_Locked;
	mutable boost::thread::id m_LockOwner;
#endif /* _DEBUG */

	mutable MutexType m_Mutex;

	friend struct ObjectLock;
};

/**
 * Compares a weak pointer with a raw pointer.
 *
 * @ingroup base
 */
template<class T>
struct WeakPtrEqual
{
private:
	const void *m_Ref; /**< The object. */

public:
	/**
	 * Constructor for the WeakPtrEqual class.
	 *
	 * @param ref The object that should be compared with the weak pointer.
	 */
	WeakPtrEqual(const void *ref) : m_Ref(ref) { }

	/**
	 * Compares the two pointers.
	 *
	 * @param wref The weak pointer.
	 * @returns true if the pointers point to the same object, false otherwise.
	 */
	bool operator()(const weak_ptr<T>& wref) const
	{
		return (wref.lock().get() == static_cast<const T *>(m_Ref));
	}
};

}

#endif /* OBJECT_H */
