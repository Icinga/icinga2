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
	inline ObjectLock(void)
		: m_Object(NULL), m_Locked(false)
	{ }

	inline ~ObjectLock(void)
	{
		Unlock();
	}

	inline ObjectLock(const Object::Ptr& object)
		: m_Object(object.get()), m_Locked(false)
	{
		if (m_Object)
			Lock();
	}

	inline ObjectLock(const Object *object)
		: m_Object(object), m_Locked(false)
	{
		if (m_Object)
			Lock();
	}

	inline static void LockMutex(const Object *object)
	{
		unsigned int it = 0;

#ifdef _WIN32
#	ifdef _WIN64
		while (likely(InterlockedCompareExchange64((LONGLONG *)&object->m_Mutex, I2MUTEX_LOCKED, I2MUTEX_UNLOCKED) != I2MUTEX_UNLOCKED)) {
#	else /* _WIN64 */
		while (likely(InterlockedCompareExchange(&object->m_Mutex, I2MUTEX_LOCKED, I2MUTEX_UNLOCKED) != I2MUTEX_UNLOCKED)) {
#	endif /* _WIN64 */
#else /* _WIN32 */
		while (likely(!__sync_bool_compare_and_swap(&object->m_Mutex, I2MUTEX_UNLOCKED, I2MUTEX_LOCKED))) {
#endif /* _WIN32 */
			if (likely(object->m_Mutex > I2MUTEX_LOCKED)) {
				boost::recursive_mutex *mtx = reinterpret_cast<boost::recursive_mutex *>(object->m_Mutex);
				mtx->lock();

				return;
			}

			Spin(it);
			it++;
		}

		boost::recursive_mutex *mtx = new boost::recursive_mutex();
		mtx->lock();
#ifdef _WIN32
#	ifdef _WIN64
		InterlockedCompareExchange64((LONGLONG *)&object->m_Mutex, reinterpret_cast<LONGLONG>(mtx), I2MUTEX_LOCKED);
#	else /* _WIN64 */
		InterlockedCompareExchange(&object->m_Mutex, reinterpret_cast<LONG>(mtx), I2MUTEX_LOCKED);
#	endif /* _WIN64 */
#else /* _WIN32 */
		__sync_bool_compare_and_swap(&object->m_Mutex, I2MUTEX_LOCKED, reinterpret_cast<uintptr_t>(mtx));
#endif /* _WIN32 */
	}

	inline void Lock(void)
	{
		ASSERT(!m_Locked && m_Object != NULL);

		LockMutex(m_Object);

		m_Locked = true;

#ifdef I2_DEBUG
#	ifdef _WIN32
		InterlockedExchange(&m_Object->m_LockOwner, GetCurrentThreadId());
#	else /* _WIN32 */
		__sync_lock_test_and_set(&m_Object->m_LockOwner, pthread_self());
#	endif /* _WIN32 */
#endif /* I2_DEBUG */
	}

	inline static void Spin(unsigned int it)
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

	inline void Unlock(void)
	{
#ifdef I2_DEBUG
		if (m_Locked) {
#	ifdef _WIN32
			InterlockedExchange(&m_Object->m_LockOwner, 0);
#	else /* _WIN32 */
			__sync_lock_release(&m_Object->m_LockOwner);
#	endif /* _WIN32 */
		}
#endif /* I2_DEBUG */

		if (m_Locked) {
			reinterpret_cast<boost::recursive_mutex *>(m_Object->m_Mutex)->unlock();
			m_Locked = false;
		}
	}

private:
	const Object *m_Object;
	bool m_Locked;
};

}

#endif /* OBJECTLOCK_H */
