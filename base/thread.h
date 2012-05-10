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

#ifndef THREAD_H
#define THREAD_H

namespace icinga
{

typedef void (*ThreadProc)(void *);

/**
 * Thread
 *
 * A wrapper around OS-specific thread functionality.
 */
class I2_BASE_API Thread
{
private:
#ifdef _WIN32
	HANDLE m_Thread;
#else
	pthread_t m_Thread;
#endif

public:
	Thread(ThreadProc callback);
	~Thread(void);

	void Join(void);
};

}

#endif /* THREAD_H */
