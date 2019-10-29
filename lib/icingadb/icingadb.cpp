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

#include "icingadb/icingadb.hpp"
#include "icingadb/icingadb-ti.cpp"
#include "icingadb/redisconnection.hpp"
#include "remote/eventqueue.hpp"
#include "base/json.hpp"
#include "icinga/checkable.hpp"
#include "icinga/host.hpp"

#include <memory>
#include <boost/algorithm/string.hpp>
#include <utility>


using namespace icinga;

//TODO Make configurable and figure out a sane default
#define MAX_EVENTS_DEFAULT 5000

REGISTER_TYPE(IcingaDB);

IcingaDB::IcingaDB()
: m_Rcon(nullptr)
{
	m_Rcon = nullptr;

	m_WorkQueue.SetName("IcingaDB");

	m_PrefixConfigObject = "icinga:config:";
	m_PrefixConfigCheckSum = "icinga:checksum:";
	m_PrefixStateObject = "icinga:config:state:";
}

/**
 * Starts the component.
 */
void IcingaDB::Start(bool runtimeCreated)
{
	ObjectImpl<IcingaDB>::Start(runtimeCreated);

	Log(LogInformation, "IcingaDB")
		<< "'" << GetName() << "' started.";

	m_ConfigDumpInProgress = false;
	m_ConfigDumpDone = false;

	m_Rcon = new RedisConnection(GetHost(), GetPort(), GetPath(), GetPassword(), GetDbIndex());
	m_Rcon->Start();

	m_WorkQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(15);
	m_ReconnectTimer->OnTimerExpired.connect([this](const Timer * const&) { ReconnectTimerHandler(); });
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	m_SubscriptionTimer = new Timer();
	m_SubscriptionTimer->SetInterval(15);
	m_SubscriptionTimer->OnTimerExpired.connect([this](const Timer * const&) { UpdateSubscriptionsTimerHandler(); });
	m_SubscriptionTimer->Start();

	m_StatsTimer = new Timer();
	m_StatsTimer->SetInterval(1);
	m_StatsTimer->OnTimerExpired.connect([this](const Timer * const&) { PublishStatsTimerHandler(); });
	m_StatsTimer->Start();

	m_WorkQueue.SetName("IcingaDB");

	boost::thread thread(&IcingaDB::HandleEvents, this);
	thread.detach();

}

void IcingaDB::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "IcingaDB", "Exception during redis query. Verify that Redis is operational.");

	Log(LogDebug, "IcingaDB")
		<< "Exception during redis operation: " << DiagnosticInformation(exp);
}

void IcingaDB::ReconnectTimerHandler()
{
	m_WorkQueue.Enqueue([this]() { TryToReconnect(); });
}

void IcingaDB::TryToReconnect()
{
	AssertOnWorkQueue();

	if (m_ConfigDumpDone)
		return;
	else
		m_Rcon->Start();

	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	UpdateSubscriptions();

	if (m_ConfigDumpInProgress || m_ConfigDumpDone)
		return;

	/* Config dump */
	m_ConfigDumpInProgress = true;
	PublishStats();

	UpdateAllConfigObjects();

	m_ConfigDumpDone = true;

	m_ConfigDumpInProgress = false;
}

void IcingaDB::UpdateSubscriptionsTimerHandler()
{
	m_WorkQueue.Enqueue([this]() { UpdateSubscriptions(); });
}

void IcingaDB::UpdateSubscriptions()
{
	AssertOnWorkQueue();

	Log(LogInformation, "IcingaDB", "Updating Redis subscriptions");

	/* TODO:
	 * Silently return in this case. Usually the RedisConnection checks for connectivity and logs in failure case.
	 * But since we expect and answer here and break Icinga in case of receiving no answer/an unexpected one we opt for
	 * better safe than sorry here. Future implementation needs to include an improved error handling and answer verification.
	 */
	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	String cursor = "0";
	String keyPrefix = "icinga:subscription:";

	do {
		Array::Ptr reply = m_Rcon->GetResultOfQuery({ "SCAN", cursor, "MATCH", keyPrefix + "*", "COUNT", "1000" });
		VERIFY(reply->GetLength() % 2u == 0u);

		cursor = reply->Get(0);

		Array::Ptr keys = reply->Get(1);
		ObjectLock oLock (keys);

		for (String key : keys) {
			if (boost::algorithm::ends_with(key, ":limit"))
				continue;

			RedisSubscriptionInfo rsi;

			if (!IcingaDB::GetSubscriptionTypes(key, rsi)) {
				Log(LogInformation, "IcingaDB")
					<< "Subscription \"" << key << "\" has no types listed.";
			} else {
				m_Subscriptions[key.SubStr(keyPrefix.GetLength())] = rsi;
			}
		}
	} while (cursor != "0");

	Log(LogInformation, "IcingaDB")
		<< "Current Redis event subscriptions: " << m_Subscriptions.size();
}

bool IcingaDB::GetSubscriptionTypes(String key, RedisSubscriptionInfo& rsi)
{
	try {
		Array::Ptr redisReply = m_Rcon->GetResultOfQuery({ "SMEMBERS", key });

		if (redisReply->GetLength() == 0)
			return false;

		{
			ObjectLock oLock (redisReply);

			for (String member : redisReply) {
				rsi.EventTypes.insert(member);
			}
		}

		Log(LogInformation, "IcingaDB")
			<< "Subscriber Info - Key: " << key << " Value: " << Value(Array::FromSet(rsi.EventTypes));

	} catch (const std::exception& ex) {
		Log(LogWarning, "IcingaDB")
			<< "Invalid Redis subscriber info for subscriber '" << key << "': " << DiagnosticInformation(ex);

		return false;
	}

	return true;
}

void IcingaDB::PublishStatsTimerHandler(void)
{
	m_WorkQueue.Enqueue([this]() { PublishStats(); });
}

void IcingaDB::PublishStats()
{
	AssertOnWorkQueue();

	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	Dictionary::Ptr status = GetStats();
	status->Set("config_dump_in_progress", m_ConfigDumpInProgress);
	String jsonStats = JsonEncode(status);

	m_Rcon->FireAndForgetQuery({ "PUBLISH", "icinga:stats", jsonStats });
}

void IcingaDB::HandleEvents()
{
	String queueName = Utility::NewUniqueID();
	EventQueue::Ptr queue = new EventQueue(queueName);
	EventQueue::Register(queueName, queue);

	std::set<String> types;
	types.insert("CheckResult");
	types.insert("StateChange");
	types.insert("Notification");
	types.insert("AcknowledgementSet");
	types.insert("AcknowledgementCleared");
	types.insert("CommentAdded");
	types.insert("CommentRemoved");
	types.insert("DowntimeAdded");
	types.insert("DowntimeRemoved");
	types.insert("DowntimeStarted");
	types.insert("DowntimeTriggered");

	queue->SetTypes(types);

	queue->AddClient(this);

	for (;;) {
		Dictionary::Ptr event = queue->WaitForEvent(this);

		if (!event)
			continue;

		m_WorkQueue.Enqueue([this, event]() { SendEvent(event); });
	}

	queue->RemoveClient(this);
	EventQueue::UnregisterIfUnused(queueName, queue);
}

void IcingaDB::HandleEvent(const Dictionary::Ptr& event)
{
	AssertOnWorkQueue();

	for (const std::pair<String, RedisSubscriptionInfo>& kv : m_Subscriptions) {
		const auto& name = kv.first;
		const auto& rsi = kv.second;

		if (rsi.EventTypes.find(event->Get("type")) == rsi.EventTypes.end())
			continue;

		String body = JsonEncode(event);

		double maxExists = m_Rcon->GetResultOfQuery({ "EXISTS", "icinga:subscription:" + name + ":limit" });

		long maxEvents = MAX_EVENTS_DEFAULT;
		if (maxExists != 0) {
			String redisReply = m_Rcon->GetResultOfQuery({ "GET", "icinga:subscription:" + name + ":limit"});

			Log(LogInformation, "IcingaDB")
				<< "Got limit " << redisReply << " for " << name;

			maxEvents = Convert::ToLong(redisReply);
		}

		m_Rcon->FireAndForgetQueries({
			{ "MULTI" },
			{ "LPUSH", "icinga:event:" + name, body },
			{ "LTRIM", "icinga:event:" + name, "0", String(maxEvents - 1)},
			{ "EXEC" }});
	}
}

void IcingaDB::SendEvent(const Dictionary::Ptr& event)
{
	AssertOnWorkQueue();

	if (!m_Rcon || !m_Rcon->IsConnected())
		return;

	String type = event->Get("type");

	if (type == "CheckResult") {
		Checkable::Ptr checkable;
		if (event->Contains("service")) {
			checkable = Service::GetByNamePair(event->Get("host"), event->Get("service"));
		} else {
			checkable = Host::GetByName(event->Get("host"));
		}
		// Update State for icingaweb
		m_WorkQueue.Enqueue([this, checkable]() { UpdateState(checkable); });
	}

	if (type.Contains("Acknowledgement")) {
		Checkable::Ptr checkable;

		if (event->Contains("service")) {
			checkable = Service::GetByNamePair(event->Get("host"), event->Get("service"));
			event->Set("service_id", GetObjectIdentifier(checkable));
		} else {
			checkable = Host::GetByName(event->Get("host"));
			event->Set("host_id", GetObjectIdentifier(checkable));
		}

		if (type == "AcknowledgementSet") {
			Timestamp entry = 0;
			Comment::Ptr AckComment;
			for (const Comment::Ptr& c : checkable->GetComments()) {
				if (c->GetEntryType() == CommentAcknowledgement) {
					if (c->GetEntryTime() > entry) {
						entry = c->GetEntryTime();
						AckComment = c;
						StateChangeHandler(checkable);
					}
				}
			}
			event->Set("comment_id", GetObjectIdentifier(AckComment));
		}
	}

	String body = JsonEncode(event);

//	Log(LogInformation, "IcingaDB")
//		<< "Sending event \"" << body << "\"";

	m_Rcon->FireAndForgetQueries({
		{ "PUBLISH", "icinga:event:all", body },
		{ "PUBLISH", "icinga:event:" + event->Get("type"), body }});
}

void IcingaDB::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "IcingaDB")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<IcingaDB>::Stop(runtimeRemoved);
}

void IcingaDB::AssertOnWorkQueue()
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}
