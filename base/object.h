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

/**
 * Base class for all heap-allocated objects. At least one of its methods
 * has to be virtual for RTTI to work.
 */
class I2_BASE_API Object : public enable_shared_from_this<Object>
{
private:
	Object(const Object& other);
	Object operator=(const Object& rhs);

protected:
	Object(void);
	virtual ~Object(void);

public:
	typedef shared_ptr<Object> Ptr;
	typedef weak_ptr<Object> WeakPtr;
};

template<class T>
struct weak_ptr_eq_raw
{
private:
	const void *m_Ref;

public:
	weak_ptr_eq_raw(const void *ref) : m_Ref(ref) { }

	bool operator()(const weak_ptr<T>& wref) const
	{
		return (wref.lock().get() == (const T *)m_Ref);
	}
};

}

#endif /* OBJECT_H */
