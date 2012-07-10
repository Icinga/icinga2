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

deque<Event::Ptr> Event::m_Events;
condition_variable Event::m_EventAvailable;
mutex Event::m_Mutex;

bool Event::Wait(vector<Event::Ptr> *events, const system_time& wait_until)
{
	mutex::scoped_lock lock(m_Mutex);

	while (m_Events.empty()) {
		if (!m_EventAvailable.timed_wait(lock, wait_until))
			return false;
	}
	
	vector<Event::Ptr> result;
	std::copy(m_Events.begin(), m_Events.end(), back_inserter(*events));
	m_Events.clear();

	return true;
}

void Event::Post(const Event::Ptr& ev)
{
	if (Application::IsMainThread()) {
		ev->OnEventDelivered();
		return;
	}

	{
		mutex::scoped_lock lock(m_Mutex);
		m_Events.push_back(ev);
		m_EventAvailable.notify_all();
	}
}
