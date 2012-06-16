/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

namespace icinga
{

class SharedPtrHolder;

/**
 * Base class for all heap-allocated objects. At least one of its methods
 * has to be virtual for RTTI to work.
 *
 * @ingroup base
 */
class I2_BASE_API Object : public enable_shared_from_this<Object>, public boost::signals::trackable
{
public:
	typedef shared_ptr<Object> Ptr;
	typedef weak_ptr<Object> WeakPtr;

	static void ClearHeldObjects(void);

protected:
	Object(void);
	virtual ~Object(void);

	void Hold(void);

	SharedPtrHolder GetSelf(void);

private:
	Object(const Object& other);
	Object operator=(const Object& rhs);

	static vector<Object::Ptr> m_HeldObjects;
};

/**
 * Holds a shared pointer and provides support for implicit upcasts.
 */
class SharedPtrHolder
{
public:
	explicit SharedPtrHolder(const shared_ptr<Object>& object)
		: m_Object(object)
	{ }

	template<typename T>
	operator shared_ptr<T>(void) const
	{
#ifdef _DEBUG
		shared_ptr<T> other = dynamic_pointer_cast<T>(m_Object);
		assert(other);
#else /* _DEBUG */
		shared_ptr<T> other = static_pointer_cast<T>(m_Object);
#endif /* _DEBUG */

		return other;
	}

	template<typename T>
	operator weak_ptr<T>(void) const
	{
		return static_cast<shared_ptr<T> >(*this);
	}

private:
	shared_ptr<Object> m_Object;
};

/**
 * Compares a weak pointer with a raw pointer.
 */
template<class T>
struct WeakPtrEqual
{
private:
	const void *m_Ref;

public:
	WeakPtrEqual(const void *ref) : m_Ref(ref) { }

	/**
	 * Compares the two pointers.
	 *
	 * @param wref The weak pointer.
	 * @returns true if the pointers point to the same object, false otherwise.
	 */
	bool operator()(const weak_ptr<T>& wref) const
	{
		return (wref.lock().get() == (const T *)m_Ref);
	}
};

}

#endif /* OBJECT_H */
