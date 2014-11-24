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

#include "base/thinmutex.hpp"
#include "base/timer.hpp"
#include "base/convert.hpp"
#include "base/logger.hpp"

using namespace icinga;

/**
 * Locks the mutex and inflates the lock.
 */
void ThinMutex::LockSlowPath(void)
{
	unsigned int it = 0;

#ifdef _WIN32
#	ifdef _WIN64
	while (InterlockedCompareExchange64(&m_Data, THINLOCK_LOCKED, THINLOCK_UNLOCKED) != THINLOCK_UNLOCKED) {
#	else /* _WIN64 */
	while (InterlockedCompareExchange(&m_Data, THINLOCK_LOCKED, THINLOCK_UNLOCKED) != THINLOCK_UNLOCKED) {
#	endif /* _WIN64 */
#else /* _WIN32 */
	while (!__sync_bool_compare_and_swap(&m_Data, THINLOCK_UNLOCKED, THINLOCK_LOCKED)) {
#endif /* _WIN32 */
		if (m_Data > THINLOCK_LOCKED) {
			boost::mutex *mtx = reinterpret_cast<boost::mutex *>(m_Data);
			mtx->lock();

			return;
		}

		Spin(it);
		it++;
	}

	boost::mutex *mtx = new boost::mutex();
	mtx->lock();
#ifdef _WIN32
#	ifdef _WIN64
	InterlockedCompareExchange64(&m_Data, reinterpret_cast<LONGLONG>(mtx), THINLOCK_LOCKED);
#	else /* _WIN64 */
	InterlockedCompareExchange(&m_Data, reinterpret_cast<LONG>(mtx), THINLOCK_LOCKED);
#	endif /* _WIN64 */
#else /* _WIN32 */
	__sync_bool_compare_and_swap(&m_Data, THINLOCK_LOCKED, reinterpret_cast<uintptr_t>(mtx));
#endif /* _WIN32 */
}

