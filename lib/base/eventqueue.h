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

#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include <boost/function.hpp>
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>

namespace icinga
{

class Timer;

/**
 * An event queue.
 *
 * @ingroup base
 */
class I2_BASE_API EventQueue
{
public:
	typedef boost::function<void ()> Callback;

	EventQueue(void);
	~EventQueue(void);

	void Stop(void);
	void Join(void);

	void Post(const Callback& callback);

private:
	boost::thread_group m_Threads;

	boost::mutex m_Mutex;
	boost::condition_variable m_CV;

	bool m_Stopped;
	stack<Callback> m_Events;

	void QueueThreadProc(void);
	void ReportThreadProc(void);
};

}

#endif /* EVENTQUEUE_H */
