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

/**
 * Default constructor for the Object class.
 */
Object::Object(void)
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

/**
 * Returns the mutex that must be held while calling non-static methods
 * which have not been explicitly marked as thread-safe.
 *
 * @returns The object's mutex.
 * @threadsafety Always.
 */
recursive_mutex& Object::GetMutex(void) const
{
	return m_Mutex;
}
