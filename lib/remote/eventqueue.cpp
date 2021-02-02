/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "config/configcompiler.hpp"
#include "remote/eventqueue.hpp"
#include "remote/filterutility.hpp"
#include "base/io-engine.hpp"
#include "base/singleton.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include <boost/asio/spawn.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <utility>

using namespace icinga;

EventQueue::EventQueue(String name)
	: m_Name(std::move(name))
{ }

bool EventQueue::CanProcessEvent(const String& type) const
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	return m_Types.find(type) != m_Types.end();
}

void EventQueue::ProcessEvent(const Dictionary::Ptr& event)
{
	Namespace::Ptr frameNS = new Namespace();
	ScriptFrame frame(true, frameNS);
	frame.Sandboxed = true;

	try {
		if (!FilterUtility::EvaluateFilter(frame, m_Filter.get(), event, "event"))
			return;
	} catch (const std::exception& ex) {
		Log(LogWarning, "EventQueue")
			<< "Error occurred while evaluating event filter for queue '" << m_Name << "': " << DiagnosticInformation(ex);
		return;
	}

	std::unique_lock<std::mutex> lock(m_Mutex);

	typedef std::pair<void *const, std::deque<Dictionary::Ptr> > kv_pair;
	for (kv_pair& kv : m_Events) {
		kv.second.push_back(event);
	}

	m_CV.notify_all();
}

void EventQueue::AddClient(void *client)
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	auto result = m_Events.insert(std::make_pair(client, std::deque<Dictionary::Ptr>()));
	ASSERT(result.second);

#ifndef I2_DEBUG
	(void)result;
#endif /* I2_DEBUG */
}

void EventQueue::RemoveClient(void *client)
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	m_Events.erase(client);
}

void EventQueue::UnregisterIfUnused(const String& name, const EventQueue::Ptr& queue)
{
	std::unique_lock<std::mutex> lock(queue->m_Mutex);

	if (queue->m_Events.empty())
		Unregister(name);
}

void EventQueue::SetTypes(const std::set<String>& types)
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	m_Types = types;
}

void EventQueue::SetFilter(std::unique_ptr<Expression> filter)
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	m_Filter.swap(filter);
}

Dictionary::Ptr EventQueue::WaitForEvent(void *client, double timeout)
{
	std::unique_lock<std::mutex> lock(m_Mutex);

	for (;;) {
		auto it = m_Events.find(client);
		ASSERT(it != m_Events.end());

		if (!it->second.empty()) {
			Dictionary::Ptr result = *it->second.begin();
			it->second.pop_front();
			return result;
		}

		if (m_CV.wait_for(lock, std::chrono::duration<double>(timeout)) == std::cv_status::timeout)
			return nullptr;
	}
}

std::vector<EventQueue::Ptr> EventQueue::GetQueuesForType(const String& type)
{
	EventQueueRegistry::ItemMap queues = EventQueueRegistry::GetInstance()->GetItems();

	std::vector<EventQueue::Ptr> availQueues;

	typedef std::pair<String, EventQueue::Ptr> kv_pair;
	for (const kv_pair& kv : queues) {
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

EventQueueRegistry *EventQueueRegistry::GetInstance()
{
	return Singleton<EventQueueRegistry>::GetInstance();
}

std::mutex EventsInbox::m_FiltersMutex;
std::map<String, EventsInbox::Filter> EventsInbox::m_Filters ({{"", EventsInbox::Filter{1, Expression::Ptr()}}});

EventsRouter EventsRouter::m_Instance;

EventsInbox::EventsInbox(String filter, const String& filterSource)
	: m_Timer(IoEngine::Get().GetIoContext())
{
	std::unique_lock<std::mutex> lock (m_FiltersMutex);
	m_Filter = m_Filters.find(filter);

	if (m_Filter == m_Filters.end()) {
		lock.unlock();

		auto expr (ConfigCompiler::CompileText(filterSource, filter));

		lock.lock();

		m_Filter = m_Filters.find(filter);

		if (m_Filter == m_Filters.end()) {
			m_Filter = m_Filters.emplace(std::move(filter), Filter{1, Expression::Ptr(expr.release())}).first;
		} else {
			++m_Filter->second.Refs;
		}
	} else {
		++m_Filter->second.Refs;
	}
}

EventsInbox::~EventsInbox()
{
	std::unique_lock<std::mutex> lock (m_FiltersMutex);

	if (!--m_Filter->second.Refs) {
		m_Filters.erase(m_Filter);
	}
}

const Expression::Ptr& EventsInbox::GetFilter()
{
	return m_Filter->second.Expr;
}

void EventsInbox::Push(Dictionary::Ptr event)
{
	std::unique_lock<std::mutex> lock (m_Mutex);

	m_Queue.emplace(std::move(event));
	m_Timer.expires_at(boost::posix_time::neg_infin);
}

Dictionary::Ptr EventsInbox::Shift(boost::asio::yield_context yc, double timeout)
{
	std::unique_lock<std::mutex> lock (m_Mutex, std::defer_lock);

	m_Timer.expires_at(boost::posix_time::neg_infin);

	{
		boost::system::error_code ec;

		while (!lock.try_lock()) {
			m_Timer.async_wait(yc[ec]);
		}
	}

	if (m_Queue.empty()) {
		m_Timer.expires_from_now(boost::posix_time::milliseconds((unsigned long)(timeout * 1000.0)));
		lock.unlock();

		{
			boost::system::error_code ec;
			m_Timer.async_wait(yc[ec]);

			while (!lock.try_lock()) {
				m_Timer.async_wait(yc[ec]);
			}
		}

		if (m_Queue.empty()) {
			return nullptr;
		}
	}

	auto event (std::move(m_Queue.front()));
	m_Queue.pop();
	return std::move(event);
}

EventsSubscriber::EventsSubscriber(std::set<EventType> types, String filter, const String& filterSource)
	: m_Types(std::move(types)), m_Inbox(new EventsInbox(std::move(filter), filterSource))
{
	EventsRouter::GetInstance().Subscribe(m_Types, m_Inbox);
}

EventsSubscriber::~EventsSubscriber()
{
	EventsRouter::GetInstance().Unsubscribe(m_Types, m_Inbox);
}

const EventsInbox::Ptr& EventsSubscriber::GetInbox()
{
	return m_Inbox;
}

EventsFilter::EventsFilter(std::map<Expression::Ptr, std::set<EventsInbox::Ptr>> inboxes)
	: m_Inboxes(std::move(inboxes))
{
}

EventsFilter::operator bool()
{
	return !m_Inboxes.empty();
}

void EventsFilter::Push(Dictionary::Ptr event)
{
	for (auto& perFilter : m_Inboxes) {
		if (perFilter.first) {
			ScriptFrame frame(true, new Namespace());
			frame.Sandboxed = true;

			try {
				if (!FilterUtility::EvaluateFilter(frame, perFilter.first.get(), event, "event")) {
					continue;
				}
			} catch (const std::exception& ex) {
				Log(LogWarning, "EventQueue")
					<< "Error occurred while evaluating event filter for queue: " << DiagnosticInformation(ex);
				continue;
			}
		}

		for (auto& inbox : perFilter.second) {
			inbox->Push(event);
		}
	}
}

EventsRouter& EventsRouter::GetInstance()
{
	return m_Instance;
}

void EventsRouter::Subscribe(const std::set<EventType>& types, const EventsInbox::Ptr& inbox)
{
	const auto& filter (inbox->GetFilter());
	std::unique_lock<std::mutex> lock (m_Mutex);

	for (auto type : types) {
		auto perType (m_Subscribers.find(type));

		if (perType == m_Subscribers.end()) {
			perType = m_Subscribers.emplace(type, decltype(perType->second)()).first;
		}

		auto perFilter (perType->second.find(filter));

		if (perFilter == perType->second.end()) {
			perFilter = perType->second.emplace(filter, decltype(perFilter->second)()).first;
		}

		perFilter->second.emplace(inbox);
	}
}

void EventsRouter::Unsubscribe(const std::set<EventType>& types, const EventsInbox::Ptr& inbox)
{
	const auto& filter (inbox->GetFilter());
	std::unique_lock<std::mutex> lock (m_Mutex);

	for (auto type : types) {
		auto perType (m_Subscribers.find(type));

		if (perType != m_Subscribers.end()) {
			auto perFilter (perType->second.find(filter));

			if (perFilter != perType->second.end()) {
				perFilter->second.erase(inbox);

				if (perFilter->second.empty()) {
					perType->second.erase(perFilter);
				}
			}

			if (perType->second.empty()) {
				m_Subscribers.erase(perType);
			}
		}
	}
}

EventsFilter EventsRouter::GetInboxes(EventType type)
{
	std::unique_lock<std::mutex> lock (m_Mutex);

	auto perType (m_Subscribers.find(type));

	if (perType == m_Subscribers.end()) {
		return EventsFilter({});
	}

	return EventsFilter(perType->second);
}
