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

#ifndef MUTEX_H
#define MUTEX_H

namespace icinga
{

/**
 * A wrapper around OS-specific mutex functionality.
 *
 * @ingroup base
 */
class I2_BASE_API Mutex
{
private:
#ifdef _WIN32
	CRITICAL_SECTION m_Mutex;
#else /* _WIN32 */
	pthread_mutex_t m_Mutex;
#endif /* _WIN32 */

public:
	Mutex(void);
	~Mutex(void);

	bool TryEnter(void);
	void Enter(void);
	void Exit(void);

#ifdef _WIN32
	CRITICAL_SECTION *Get(void);
#else /* _WIN32 */
	pthread_mutex_t *Get(void);
#endif /* _WIN32 */
};

}

#endif /* MUTEX_H */
