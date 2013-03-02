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

#include "i2-base.h"

using namespace icinga;

#ifdef _DEBUG
boost::mutex Object::m_DebugMutex;
#endif /* _DEBUG */

/**
 * Default constructor for the Object class.
 */
Object::Object(void)
	: m_LockCount(0)
{ }

/**
 * Destructor for the Object class.
 */
Object::~Object(void)
{ }

/**
 * Returns a reference-counted pointer to this object.
 *
 * @returns A shared_ptr object that points to this object
 * @threadsafety Always.
 */
Object::SharedPtrHolder Object::GetSelf(void)
{
	ObjectLock olock(this);

	return Object::SharedPtrHolder(shared_from_this());
}

#ifdef _DEBUG
/**
 * Checks if the calling thread owns the lock on this object or is currently
 * in the constructor or destructor and therefore implicitly owns the lock.
 *
 * @returns True if the calling thread owns the lock, false otherwise.
 */
bool Object::OwnsLock(void) const
{
	boost::mutex::scoped_lock lock(m_DebugMutex);

	if (m_LockCount == 0 || m_LockOwner != boost::this_thread::get_id()) {
		try {
			shared_from_this();
		} catch (const boost::bad_weak_ptr& ex) {
			/* There's no shared_ptr to this object. Either someone created the object
			 * directly (e.g. on the stack) or we're in the constructor or destructor. Not holding the lock is ok here. */
			return true;
		}

		return false;
	}

	return true;
}
#endif /* _DEBUG */
