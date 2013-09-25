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

#include "base/objectlock.h"
#include "base/debug.h"

using namespace icinga;

ObjectLock::ObjectLock(void)
	: m_Object(NULL), m_Lock()
{ }

ObjectLock::~ObjectLock(void)
{
	Unlock();
}

ObjectLock::ObjectLock(const Object::Ptr& object)
	: m_Object(object.get()), m_Lock()
{
	if (m_Object)
		Lock();
}

ObjectLock::ObjectLock(const Object *object)
	: m_Object(object), m_Lock()
{
	if (m_Object)
		Lock();
}

void ObjectLock::Lock(void)
{
	ASSERT(!m_Lock.owns_lock() && m_Object != NULL);
	ASSERT(!m_Object->OwnsLock());

	m_Lock = Object::MutexType::scoped_lock(m_Object->m_Mutex);

#ifdef _DEBUG
	{
		boost::mutex::scoped_lock lock(Object::m_DebugMutex);
		m_Object->m_Locked = true;
		m_Object->m_LockOwner = boost::this_thread::get_id();
	}
#endif /* _DEBUG */
}

void ObjectLock::Unlock(void)
{
#ifdef _DEBUG
	{
		boost::mutex::scoped_lock lock(Object::m_DebugMutex);

		if (m_Lock.owns_lock())
			m_Object->m_Locked = false;
	}
#endif /* _DEBUG */

	m_Lock = Object::MutexType::scoped_lock();
}
