/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/object.hpp"
#include "base/value.hpp"

using namespace icinga;

#ifdef _DEBUG
boost::mutex Object::m_DebugMutex;
#endif /* _DEBUG */

/**
 * Default constructor for the Object class.
 */
Object::Object(void)
#ifdef _DEBUG
	: m_Locked(false)
#endif /* _DEBUG */
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
 */
Object::SharedPtrHolder Object::GetSelf(void)
{
	return Object::SharedPtrHolder(shared_from_this());
}

#ifdef _DEBUG
/**
 * Checks if the calling thread owns the lock on this object.
 *
 * @returns True if the calling thread owns the lock, false otherwise.
 */
bool Object::OwnsLock(void) const
{
	boost::mutex::scoped_lock lock(m_DebugMutex);

	return (m_Locked && m_LockOwner == boost::this_thread::get_id());
}
#endif /* _DEBUG */

Object::SharedPtrHolder::operator Value(void) const
{
	return m_Object;
}

const Type *Object::GetReflectionType(void) const
{
	return NULL;
}

void Object::SetField(int, const Value&)
{
	BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

Value Object::GetField(int) const
{
	BOOST_THROW_EXCEPTION(std::runtime_error("Invalid field ID."));
}

