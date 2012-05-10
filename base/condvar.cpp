/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#include "i2-base.h"

using namespace icinga;

/**
 * CondVar
 *
 * Constructor for the CondVar class.
 */
CondVar::CondVar(void)
{
#ifdef _WIN32
	InitializeConditionVariable(&m_CondVar);
#else /* _WIN32 */
	
#endif /* _WIN32 */
}

/**
 * ~CondVar
 *
 * Destructor for the CondVar class.
 */
CondVar::~CondVar(void)
{
#ifdef _WIN32
	/* nothing to do here */
#else /* _WIN32 */

#endif /* _WIN32 */
}

/**
 * Wait
 *
 * Waits for the condition variable to be signaled. Releases the specified mutex
 * before it begins to wait and re-acquires the mutex after waiting.
 *
 * @param mtx The mutex that should be released during waiting.
 */
void CondVar::Wait(Mutex& mtx)
{
#ifdef _WIN32
	SleepConditionVariableCS(&m_CondVar, mtx.Get(), INFINITE);
#else /* _WIN32 */
	pthread_cond_wait(&m_CondVar, mtx.Get());
#endif /* _WIN32 */
}

/**
 * Signal
 *
 * Wakes up at least one waiting thread.
 */
void CondVar::Signal(void)
{
#ifdef _WIN32
	WakeConditionVariable(&m_CondVar);
#else /* _WIN32 */
	pthread_cond_signal(&m_CondVar);
#endif /* _WIN32 */
}

/**
 * Broadcast
 *
 * Wakes up all waiting threads.
 */
void CondVar::Broadcast(void)
{
#ifdef _WIN32
	WakeAllConditionVariable(&m_CondVar);
#else /* _WIN32 */
	pthread_cond_broadcast(&m_CondVar);
#endif /* _WIN32 */
}

/**
 * Get
 *
 * Retrieves the platform-specific condition variable handle.
 *
 * @returns The platform-specific condition variable handle.
 */
#ifdef _WIN32
CONDITION_VARIABLE *CondVar::Get(void)
#else /* _WIN32 */
pthread_cond_t *CondVar::Get(void)
#endif /* _WIN32 */
{
	return &m_CondVar;
}
