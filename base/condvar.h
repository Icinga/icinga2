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

#ifndef CONDVAR_H
#define CONDVAR_H

namespace icinga
{

/**
 * A wrapper around OS-specific condition variable functionality.
 */
class I2_BASE_API CondVar
{
private:
#ifdef _WIN32
	CONDITION_VARIABLE m_CondVar;
#else /* _WIN32 */
	pthread_cond_t m_CondVar;
#endif /* _WIN32 */

public:
	CondVar(void);
	~CondVar(void);

	void Wait(Mutex& mtx);
	void Signal(void);
	void Broadcast(void);

#ifdef _WIN32
	CONDITION_VARIABLE *Get(void);
#else /* _WIN32 */
	pthread_cond_t *Get(void);
#endif /* _WIN32 */
};

}

#endif /* CONDVAR_H */
