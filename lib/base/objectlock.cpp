/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/objectlock.hpp"
#include <thread>

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

/**
 * Constructs a lock for the given object without locking it immediately.
 *
 * The user must call Lock() explicitly when needed.
 *
 * @param object The object to lock.
 */
ObjectLock::ObjectLock(const Object::Ptr& object, std::defer_lock_t)
	: m_Object(object.get()), m_Locked(false)
{
}

ObjectLock::ObjectLock(const Object *object)
	: m_Object(object), m_Locked(false)
{
	if (m_Object)
		Lock();
}

void ObjectLock::Lock()
{
	ASSERT(!m_Locked && m_Object);

	m_Object->m_Mutex.lock();

	m_Locked = true;

#ifdef I2_DEBUG
	if (++m_Object->m_LockCount == 1u) {
		m_Object->m_LockOwner.store(std::this_thread::get_id());
	}
#endif /* I2_DEBUG */
}

void ObjectLock::Unlock()
{
#ifdef I2_DEBUG
	if (m_Locked && !--m_Object->m_LockCount) {
		m_Object->m_LockOwner.store(decltype(m_Object->m_LockOwner.load())());
	}
#endif /* I2_DEBUG */

	if (m_Locked) {
		m_Object->m_Mutex.unlock();
		m_Locked = false;
	}
}

/**
 * Returns true if the object is locked, false otherwise.
 *
 * This operator allows using ObjectLock in boolean contexts.
 *
 * @returns true if the object is locked, false otherwise.
 */
ObjectLock::operator bool() const
{
	return m_Locked;
}
