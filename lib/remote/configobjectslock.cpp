// SPDX-FileCopyrightText: 2022 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "remote/configobjectslock.hpp"

#ifndef _WIN32
#include "base/shared-memory.hpp"
#include <boost/interprocess/sync/lock_options.hpp>
#endif /* _WIN32 */

using namespace icinga;

#ifndef _WIN32

// On *nix one process may write config objects while another is loading the config, so this uses IPC.
static SharedMemory<boost::interprocess::interprocess_sharable_mutex> l_ConfigObjectsMutex;

ConfigObjectsExclusiveLock::ConfigObjectsExclusiveLock()
	: m_Lock(l_ConfigObjectsMutex.Get())
{
}

ConfigObjectsSharedLock::ConfigObjectsSharedLock(std::try_to_lock_t)
	: m_Lock(l_ConfigObjectsMutex.Get(), boost::interprocess::try_to_lock)
{
}

#endif /* _WIN32 */

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
