/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#ifndef OBJECTLOCK_H
#define OBJECTLOCK_H

#include "base/object.hpp"

#define I2MUTEX_UNLOCKED 0
#define I2MUTEX_LOCKED 1

namespace icinga
{

/**
 * A scoped lock for Objects.
 */
struct I2_BASE_API ObjectLock
{
public:
	ObjectLock(void)
		: m_Object(nullptr), m_Locked(false)
	{ }

	~ObjectLock(void)
	{
		Unlock();
	}

	ObjectLock(const Object::Ptr& object)
		: m_Object(object.get()), m_Locked(false)
	{
		if (m_Object)
			Lock();
	}

	ObjectLock(const Object *object)
		: m_Object(object), m_Locked(false)
	{
		if (m_Object)
			Lock();
	}

	static void LockMutex(const Object *object)
	{
		unsigned int it = 0;

		uintptr_t tmp = I2MUTEX_UNLOCKED;

		while (!object->m_Mutex.compare_exchange_weak(tmp, I2MUTEX_LOCKED)) {
			if (tmp > I2MUTEX_LOCKED) {
				boost::recursive_mutex *mtx = reinterpret_cast<boost::recursive_mutex *>(tmp);
				mtx->lock();

				return;
			}

			tmp = I2MUTEX_UNLOCKED;

			Spin(it);
			it++;
		}

		boost::recursive_mutex *mtx = new boost::recursive_mutex();
		mtx->lock();

		object->m_Mutex.store(reinterpret_cast<uintptr_t>(mtx));
	}

	void Lock(void)
	{
		ASSERT(!m_Locked && m_Object);

		LockMutex(m_Object);

		m_Locked = true;

#ifdef I2_DEBUG
#	ifdef _WIN32
		m_Object->m_LockOwner = GetCurrentThreadId();
#	else /* _WIN32 */
		m_Object->m_LockOwner = pthread_self();
#	endif /* _WIN32 */
#endif /* I2_DEBUG */
	}

	static void Spin(unsigned int it)
	{
		if (it < 8) {
			/* Do nothing. */
		}
#ifdef SPIN_PAUSE
		else if (it < 16) {
			SPIN_PAUSE();
		}
#endif /* SPIN_PAUSE */
		else {
#ifdef _WIN32
			Sleep(0);
#else /* _WIN32 */
			sched_yield();
#endif /* _WIN32 */
		}
	}

	void Unlock(void)
	{
		if (m_Locked) {
#ifdef I2_DEBUG
			memset(&m_Object->m_LockOwner, 0, sizeof(m_Object->m_LockOwner));
#endif /* I2_DEBUG */

			uintptr_t mtx = m_Object->m_Mutex.load();

			reinterpret_cast<boost::recursive_mutex *>(mtx)->unlock();

			m_Locked = false;
		}
	}

private:
	const Object *m_Object;
	bool m_Locked;
};

}

#endif /* OBJECTLOCK_H */
