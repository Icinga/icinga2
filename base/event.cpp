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

vector<Event> Event::m_Events;
condition_variable Event::m_EventAvailable;
mutex Event::m_Mutex;

Event::Event(const function<void ()>& callback)
	: m_Callback(callback)
{ }

void Event::ProcessEvents(const system_time& wait_until)
{
	vector<Event> events;

	{
		mutex::scoped_lock lock(m_Mutex);

		while (m_Events.empty()) {
			if (!m_EventAvailable.timed_wait(lock, wait_until))
				return;
		}

		events.swap(m_Events);
	}

	vector<Event>::iterator it;
	for (it = events.begin(); it != events.end(); it++)
		it->m_Callback();
}

void Event::Post(const function<void ()>& callback)
{
	if (Application::IsMainThread()) {
		callback();
		return;
	}

	Event ev(callback);

	{
		mutex::scoped_lock lock(m_Mutex);
		m_Events.push_back(ev);
		m_EventAvailable.notify_all();
	}
}
