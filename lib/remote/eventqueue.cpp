// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "config/configcompiler.hpp"
#include "remote/eventqueue.hpp"
#include "remote/filterutility.hpp"
#include "base/io-engine.hpp"
#include "base/logger.hpp"
#include <boost/asio/spawn.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/date_time/posix_time/ptime.hpp>
#include <boost/system/error_code.hpp>
#include <utility>

using namespace icinga;

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
	m_Timer.expires_at(boost::asio::steady_timer::time_point::min());
}

Dictionary::Ptr EventsInbox::Shift(boost::asio::yield_context yc, std::chrono::milliseconds timeout)
{
	std::unique_lock<std::mutex> lock (m_Mutex, std::defer_lock);

	m_Timer.expires_at(boost::asio::steady_timer::time_point::min());

	{
		boost::system::error_code ec;

		while (!lock.try_lock()) {
			m_Timer.async_wait(yc[ec]);
		}
	}

	if (m_Queue.empty()) {
		m_Timer.expires_after(timeout);
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
	return event;
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
