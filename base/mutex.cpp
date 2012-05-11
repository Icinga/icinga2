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
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-base.h"

using namespace icinga;

/**
 * Mutex
 *
 * Constructor for the Mutex class.
 */
Mutex::Mutex(void)
{
#ifdef _WIN32
	InitializeCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_init(&m_Mutex, NULL);
#endif /* _WIN32 */
}

/**
 * ~Mutex
 *
 * Destructor for the Mutex class.
 */
Mutex::~Mutex(void)
{
#ifdef _WIN32
	DeleteCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_destroy(&m_Mutex);
#endif /* _WIN32 */
}

/**
 * TryEnter
 *
 * Tries to lock the mutex. If the mutex cannot be immediatelly
 * locked the operation fails.
 *
 * @returns true if the operation succeeded, false otherwise.
 */
bool Mutex::TryEnter(void)
{
#ifdef _WIN32
	return (TryEnterCriticalSection(&m_Mutex) == TRUE);
#else /* _WIN32 */
	return pthread_mutex_trylock(&m_Mutex);
#endif /* _WIN32 */
}

/**
 * Enter
 *
 * Locks the mutex.
 */
void Mutex::Enter(void)
{
#ifdef _WIN32
	EnterCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_lock(&m_Mutex);
#endif /* _WIN32 */
}

/**
 * Exit
 *
 * Unlocks the mutex.
 */
void Mutex::Exit(void)
{
#ifdef _WIN32
	LeaveCriticalSection(&m_Mutex);
#else /* _WIN32 */
	pthread_mutex_unlock(&m_Mutex);
#endif /* _WIN32 */
}

/**
 * Get
 *
 * Retrieves the platform-specific mutex handle.
 *
 * @returns The platform-specific mutex handle.
 */
#ifdef _WIN32
CRITICAL_SECTION *Mutex::Get(void)
#else /* _WIN32 */
pthread_mutex_t *Mutex::Get(void)
#endif /* _WIN32 */
{
	return &m_Mutex;
}
