// SPDX-FileCopyrightText: 2023 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "base/type.hpp"
#include "base/string.hpp"
#include <condition_variable>
#include <map>
#include <mutex>
#include <set>

#ifndef _WIN32
#include <boost/interprocess/sync/interprocess_sharable_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/sharable_lock.hpp>
#endif /* _WIN32 */

namespace icinga
{

#ifdef _WIN32

class ConfigObjectsSharedLock
{
public:
	inline ConfigObjectsSharedLock(std::try_to_lock_t)
	{
	}

	constexpr explicit operator bool() const
	{
		return true;
	}
};

#else /* _WIN32 */

/**
 * Waits until all ConfigObjects*Lock-s have vanished. For its lifetime disallows such.
 * Keep an instance alive during reload to forbid runtime config changes!
 * This way Icinga reads a consistent config which doesn't suddenly get runtime-changed.
 *
 * @ingroup remote
 */
class ConfigObjectsExclusiveLock
{
public:
	ConfigObjectsExclusiveLock();

private:
	boost::interprocess::scoped_lock<boost::interprocess::interprocess_sharable_mutex> m_Lock;
};

/**
 * Waits until the only ConfigObjectsExclusiveLock has vanished (if any). For its lifetime disallows such.
 * Keep an instance alive during runtime config changes to delay a reload (if any)!
 * This way Icinga reads a consistent config which doesn't suddenly get runtime-changed.
 *
 * @ingroup remote
 */
class ConfigObjectsSharedLock
{
public:
	ConfigObjectsSharedLock(std::try_to_lock_t);

	inline explicit operator bool() const
	{
		return m_Lock.owns();
	}

private:
	boost::interprocess::sharable_lock<boost::interprocess::interprocess_sharable_mutex> m_Lock;
};

#endif /* _WIN32 */


/**
 * Allows you to easily lock/unlock a specific object of a given type by its name.
 *
 * That way, locking an object "this" of type Host does not affect an object "this" of
 * type "Service" nor an object "other" of type "Host".
 *
 * @ingroup remote
 */
class ObjectNameLock
{
public:
	ObjectNameLock(const Type::Ptr& ptype, const String& objName);

	~ObjectNameLock();

private:
	String m_ObjectName;
	Type::Ptr m_Type;

	static std::mutex m_Mutex;
	static std::condition_variable m_CV;
	static std::map<Type*, std::set<String>> m_LockedObjectNames;
};

}
