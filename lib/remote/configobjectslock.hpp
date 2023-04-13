/* Icinga 2 | (c) 2023 Icinga GmbH | GPLv2+ */

#pragma once

#include <mutex>

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

}
