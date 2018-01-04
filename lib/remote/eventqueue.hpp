/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "remote/httphandler.hpp"
#include "base/object.hpp"
#include "config/expression.hpp"
#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <set>
#include <map>
#include <deque>

namespace icinga
{

class EventQueue final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(EventQueue);

	EventQueue(const String& name);

	bool CanProcessEvent(const String& type) const;
	void ProcessEvent(const Dictionary::Ptr& event);
	void AddClient(void *client);
	void RemoveClient(void *client);

	void SetTypes(const std::set<String>& types);
	void SetFilter(std::unique_ptr<Expression> filter);

	Dictionary::Ptr WaitForEvent(void *client, double timeout = 5);

	static std::vector<EventQueue::Ptr> GetQueuesForType(const String& type);
	static void UnregisterIfUnused(const String& name, const EventQueue::Ptr& queue);

	static EventQueue::Ptr GetByName(const String& name);
	static void Register(const String& name, const EventQueue::Ptr& function);
	static void Unregister(const String& name);

private:
	String m_Name;

	mutable boost::mutex m_Mutex;
	boost::condition_variable m_CV;

	std::set<String> m_Types;
	std::unique_ptr<Expression> m_Filter;

	std::map<void *, std::deque<Dictionary::Ptr> > m_Events;
};

/**
 * A registry for API event queues.
 *
 * @ingroup base
 */
class EventQueueRegistry : public Registry<EventQueueRegistry, EventQueue::Ptr>
{
public:
	static EventQueueRegistry *GetInstance(void);
};

}

#endif /* EVENTQUEUE_H */
