/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include "remote/httphandler.hpp"
#include "base/object.hpp"
#include "config/expression.hpp"
#include <boost/asio/spawn.hpp>
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

	EventQueue(String name);

	bool CanProcessEvent(const String& type) const;
	void ProcessEvent(const Dictionary::Ptr& event);
	void AddClient(void *client);
	void RemoveClient(void *client);

	void SetTypes(const std::set<String>& types);
	void SetFilter(std::unique_ptr<Expression> filter);

	Dictionary::Ptr WaitForEvent(void *client, double timeout = 5);
	Dictionary::Ptr WaitForEvent(void *client, boost::asio::yield_context yc);

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
	static EventQueueRegistry *GetInstance();
};

}

#endif /* EVENTQUEUE_H */
