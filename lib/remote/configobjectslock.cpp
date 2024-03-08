/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#include "remote/configobjectslock.hpp"

using namespace icinga;

std::mutex ObjectNameLock::m_Mutex;
std::condition_variable ObjectNameLock::m_CV;
std::map<Type*, std::set<String>> ObjectNameLock::m_LockedObjectNames;

/**
 * Locks the specified object name of the given type and unlocks it upon destruction of the instance of this class.
 *
 * If it is already locked, the call blocks until the lock is released.
 *
 * @param Type::Ptr ptype The type of the object you want to lock
 * @param String objName The object name you want to lock
 */
ObjectNameLock::ObjectNameLock(const Type::Ptr& ptype, const String& objName): m_ObjectName{objName}, m_Type{ptype}
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	m_CV.wait(lock, [this]{
		auto& locked = m_LockedObjectNames[m_Type.get()];
		return locked.find(m_ObjectName) == locked.end();
	});

	// Add the object name to the locked list to block all other threads that try
	// to process a message affecting the same object.
	m_LockedObjectNames[ptype.get()].emplace(objName);
}

ObjectNameLock::~ObjectNameLock()
{
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_LockedObjectNames[m_Type.get()].erase(m_ObjectName);
	}
	m_CV.notify_all();
}
