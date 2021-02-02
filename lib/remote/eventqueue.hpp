/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef EVENTQUEUE_H
#define EVENTQUEUE_H

#include "remote/httphandler.hpp"
#include "base/object.hpp"
#include "config/expression.hpp"
#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/spawn.hpp>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <set>
#include <map>
#include <deque>
#include <queue>

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

	static std::vector<EventQueue::Ptr> GetQueuesForType(const String& type);
	static void UnregisterIfUnused(const String& name, const EventQueue::Ptr& queue);

	static EventQueue::Ptr GetByName(const String& name);
	static void Register(const String& name, const EventQueue::Ptr& function);
	static void Unregister(const String& name);

private:
	String m_Name;

	mutable std::mutex m_Mutex;
	std::condition_variable m_CV;

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

enum class EventType : uint_fast8_t
{
	AcknowledgementCleared,
	AcknowledgementSet,
	CheckResult,
	CommentAdded,
	CommentRemoved,
	DowntimeAdded,
	DowntimeRemoved,
	DowntimeStarted,
	DowntimeTriggered,
	Flapping,
	Notification,
	StateChange,
	ObjectCreated,
	ObjectDeleted,
	ObjectModified
};

class EventsInbox : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(EventsInbox);

	EventsInbox(String filter, const String& filterSource);
	EventsInbox(const EventsInbox&) = delete;
	EventsInbox(EventsInbox&&) = delete;
	EventsInbox& operator=(const EventsInbox&) = delete;
	EventsInbox& operator=(EventsInbox&&) = delete;
	~EventsInbox();

	const Expression::Ptr& GetFilter();

	void Push(Dictionary::Ptr event);
	Dictionary::Ptr Shift(boost::asio::yield_context yc, double timeout = 5);

private:
	struct Filter
	{
		std::size_t Refs;
		Expression::Ptr Expr;
	};

	static std::mutex m_FiltersMutex;
	static std::map<String, Filter> m_Filters;

	std::mutex m_Mutex;
	decltype(m_Filters.begin()) m_Filter;
	std::queue<Dictionary::Ptr> m_Queue;
	boost::asio::deadline_timer m_Timer;
};

class EventsSubscriber
{
public:
	EventsSubscriber(std::set<EventType> types, String filter, const String& filterSource);
	EventsSubscriber(const EventsSubscriber&) = delete;
	EventsSubscriber(EventsSubscriber&&) = delete;
	EventsSubscriber& operator=(const EventsSubscriber&) = delete;
	EventsSubscriber& operator=(EventsSubscriber&&) = delete;
	~EventsSubscriber();

	const EventsInbox::Ptr& GetInbox();

private:
	std::set<EventType> m_Types;
	EventsInbox::Ptr m_Inbox;
};

class EventsFilter
{
public:
	EventsFilter(std::map<Expression::Ptr, std::set<EventsInbox::Ptr>> inboxes);

	operator bool();

	void Push(Dictionary::Ptr event);

private:
	std::map<Expression::Ptr, std::set<EventsInbox::Ptr>> m_Inboxes;
};

class EventsRouter
{
public:
	static EventsRouter& GetInstance();

	void Subscribe(const std::set<EventType>& types, const EventsInbox::Ptr& inbox);
	void Unsubscribe(const std::set<EventType>& types, const EventsInbox::Ptr& inbox);
	EventsFilter GetInboxes(EventType type);

private:
	static EventsRouter m_Instance;

	EventsRouter() = default;
	EventsRouter(const EventsRouter&) = delete;
	EventsRouter(EventsRouter&&) = delete;
	EventsRouter& operator=(const EventsRouter&) = delete;
	EventsRouter& operator=(EventsRouter&&) = delete;
	~EventsRouter() = default;

	std::mutex m_Mutex;
	std::map<EventType, std::map<Expression::Ptr, std::set<EventsInbox::Ptr>>> m_Subscribers;
};

}

#endif /* EVENTQUEUE_H */
