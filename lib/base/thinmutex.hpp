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

#ifndef THINMUTEX_H
#define THINMUTEX_H

#include "base/i2-base.hpp"
#include <boost/thread/mutex.hpp>
#ifndef _WIN32
#include <sched.h>
#endif /* _WIN32 */

#if defined(_MSC_VER) && _MSC_VER >= 1310 && (defined(_M_IX86) || defined(_M_X64))
extern "C" void _mm_pause();
#pragma intrinsic(_mm_pause)
#define SPIN_PAUSE() _mm_pause()
#elif defined(__GNUC__) && (defined(__i386__) || defined(__x86_64__))
#define SPIN_PAUSE() __asm__ __volatile__("rep; nop" : : : "memory")
#endif

#define THINLOCK_UNLOCKED 0
#define THINLOCK_LOCKED 1

namespace icinga {

class I2_BASE_API ThinMutex
{
public:
	inline ThinMutex(void)
		: m_Data(THINLOCK_UNLOCKED)
	{ }

	inline ~ThinMutex(void)
	{
		if (m_Data > THINLOCK_LOCKED)
			delete reinterpret_cast<boost::mutex *>(m_Data);
	}

	inline void Lock(void)
	{
#ifdef _WIN32
#	ifdef _WIN64
		if (InterlockedCompareExchange64(&m_Data, THINLOCK_LOCKED, THINLOCK_UNLOCKED) != THINLOCK_UNLOCKED) {
#	else /* _WIN64 */
		if (InterlockedCompareExchange(&m_Data, THINLOCK_LOCKED, THINLOCK_UNLOCKED) != THINLOCK_UNLOCKED) {
#	endif /* _WIN64 */
#else /* _WIN32 */
		if (!__sync_bool_compare_and_swap(&m_Data, THINLOCK_UNLOCKED, THINLOCK_LOCKED)) {
#endif /* _WIN32 */
			LockSlowPath();
		}
	}

	inline void Unlock(void)
	{
#ifdef _WIN32
#	ifdef _WIN64
		if (InterlockedCompareExchange64(&m_Data, THINLOCK_UNLOCKED, THINLOCK_LOCKED) != THINLOCK_LOCKED) {
#	else /* _WIN64 */
		if (InterlockedCompareExchange(&m_Data, THINLOCK_UNLOCKED, THINLOCK_LOCKED) != THINLOCK_LOCKED) {
#	endif /* _WIN64 */
#else /* _WIN32 */
		if (!__sync_bool_compare_and_swap(&m_Data, THINLOCK_LOCKED, THINLOCK_UNLOCKED)) {
#endif /* _WIN32 */
			boost::mutex *mtx = reinterpret_cast<boost::mutex *>(m_Data);
			mtx->unlock();
		}
	}

	inline void Inflate(void)
	{
		LockSlowPath();
		Unlock();
	}

private:
	inline void Spin(unsigned int it)
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

private:
#ifdef _WIN32
#	ifdef _WIN64
	LONGLONG m_Data;
#	else /* _WIN64 */
	LONG m_Data;
#	endif /* _WIN64 */
#else /* _WIN32 */
	uintptr_t m_Data;
#endif /* _WIN32 */

	void LockSlowPath(void);
};

}

#endif /* THINMUTEX_H */
