/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#ifndef _WIN32

#include "base/shared-memory.hpp"
#include "remote/configobjectslock.hpp"
#include <boost/interprocess/sync/lock_options.hpp>

using namespace icinga;

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
