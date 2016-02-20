/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "remote/eventqueue.hpp"
#include "remote/filterutility.hpp"
#include "base/singleton.hpp"

using namespace icinga;

EventQueue::EventQueue(void)
    : m_Filter(NULL)
{ }

EventQueue::~EventQueue(void)
{
	delete m_Filter;
}

bool EventQueue::CanProcessEvent(const String& type) const
{
	boost::mutex::scoped_lock lock(m_Mutex);

	return m_Types.find(type) != m_Types.end();
}

void EventQueue::ProcessEvent(const Dictionary::Ptr& event)
{
	ScriptFrame frame;
	frame.Sandboxed = true;

	if (!FilterUtility::EvaluateFilter(frame, m_Filter, event, "event"))
		return;

	boost::mutex::scoped_lock lock(m_Mutex);

	typedef std::pair<void *const, std::deque<Dictionary::Ptr> > kv_pair;
	BOOST_FOREACH(kv_pair& kv, m_Events) {
		kv.second.push_back(event);
	}

	m_CV.notify_all();
}

void EventQueue::AddClient(void *client)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	typedef std::map<void *, std::deque<Dictionary::Ptr> >::iterator it_type;
	std::pair<it_type, bool> result = m_Events.insert(std::make_pair(client, std::deque<Dictionary::Ptr>()));
	ASSERT(result.second);
}

void EventQueue::RemoveClient(void *client)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	m_Events.erase(client);
}

void EventQueue::UnregisterIfUnused(const String& name, const EventQueue::Ptr& queue)
{
	boost::mutex::scoped_lock lock(queue->m_Mutex);

	if (queue->m_Events.empty())
		Unregister(name);
}

void EventQueue::SetTypes(const std::set<String>& types)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Types = types;
}

void EventQueue::SetFilter(Expression *filter)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	delete m_Filter;
	m_Filter = filter;
}

Dictionary::Ptr EventQueue::WaitForEvent(void *client, double timeout)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	for (;;) {
		std::map<void *, std::deque<Dictionary::Ptr> >::iterator it = m_Events.find(client);
		ASSERT(it != m_Events.end());

		if (!it->second.empty()) {
			Dictionary::Ptr result = *it->second.begin();
			it->second.pop_front();
			return result;
		}

		if (!m_CV.timed_wait(lock, boost::posix_time::milliseconds(timeout * 1000)))
			return Dictionary::Ptr();
	}
}


std::vector<EventQueue::Ptr> EventQueue::GetQueuesForType(const String& type)
{
	EventQueueRegistry::ItemMap queues = EventQueueRegistry::GetInstance()->GetItems();

	std::vector<EventQueue::Ptr> availQueues;

	typedef std::pair<String, EventQueue::Ptr> kv_pair;
	BOOST_FOREACH(const kv_pair& kv, queues) {
		if (kv.second->CanProcessEvent(type))
			availQueues.push_back(kv.second);
	}

	return availQueues;
}

EventQueue::Ptr EventQueue::GetByName(const String& name)
{
	return EventQueueRegistry::GetInstance()->GetItem(name);
}

void EventQueue::Register(const String& name, const EventQueue::Ptr& function)
{
	EventQueueRegistry::GetInstance()->Register(name, function);
}

void EventQueue::Unregister(const String& name)
{
	EventQueueRegistry::GetInstance()->Unregister(name);
}

EventQueueRegistry *EventQueueRegistry::GetInstance(void)
{
	return Singleton<EventQueueRegistry>::GetInstance();
}
