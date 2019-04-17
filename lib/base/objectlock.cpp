/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/objectlock.hpp"
#include <boost/thread/recursive_mutex.hpp>

using namespace icinga;

#define I2MUTEX_UNLOCKED 0
#define I2MUTEX_LOCKED 1

ObjectLock::~ObjectLock()
{
	Unlock();
}

ObjectLock::ObjectLock(const Object::Ptr& object)
	: ObjectLock(object.get())
{
}

ObjectLock::ObjectLock(const Object *object)
	: m_Object(object), m_Locked(false)
{
	if (m_Object)
		Lock();
}

void ObjectLock::LockMutex(const Object *object)
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
			auto *mtx = reinterpret_cast<boost::recursive_mutex *>(object->m_Mutex);
			mtx->lock();

			return;
		}

		Spin(it);
		it++;
	}

	auto *mtx = new boost::recursive_mutex();
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

void ObjectLock::Lock()
{
	ASSERT(!m_Locked && m_Object);

	LockMutex(m_Object);

	m_Locked = true;

#ifdef I2_DEBUG
	if (++m_Object->m_LockCount == 1u) {
#	ifdef _WIN32
		InterlockedExchange(&m_Object->m_LockOwner, GetCurrentThreadId());
#	else /* _WIN32 */
		__sync_lock_test_and_set(&m_Object->m_LockOwner, pthread_self());
#	endif /* _WIN32 */
	}
#endif /* I2_DEBUG */
}

void ObjectLock::Spin(unsigned int it)
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

void ObjectLock::Unlock()
{
#ifdef I2_DEBUG
	if (m_Locked && !--m_Object->m_LockCount) {
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
