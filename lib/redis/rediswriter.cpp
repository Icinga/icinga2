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

#include "redis/rediswriter.hpp"
#include "redis/rediswriter-ti.cpp"
#include "redis/redisconnection.hpp"
#include "remote/eventqueue.hpp"
#include "base/json.hpp"
#include "icinga/checkable.hpp"
#include "icinga/host.hpp"

#include <boost/algorithm/string.hpp>


using namespace icinga;

//TODO Make configurable and figure out a sane default
#define MAX_EVENTS_DEFAULT 5000

REGISTER_TYPE(RedisWriter);

RedisWriter::RedisWriter()
: m_Rcon(nullptr)
{
	m_Rcon = nullptr;

	m_WorkQueue.SetName("RedisWriter");

	m_PrefixConfigObject = "icinga:config:object:";
	m_PrefixConfigCheckSum = "icinga:config:checksum:";
	m_PrefixConfigCustomVar = "icinga:config:customvar:";
	m_PrefixStatusObject = "icinga:status:object:";
}

/**
 * Starts the component.
 */
void RedisWriter::Start(bool runtimeCreated)
{
	ObjectImpl<RedisWriter>::Start(runtimeCreated);

	Log(LogInformation, "RedisWriter")
		<< "'" << GetName() << "' started.";

	m_ConfigDumpInProgress = false;
	m_ConfigDumpDone = false;

	m_Rcon = new RedisConnection(GetHost(), GetPort(), GetPath(), GetPassword(), GetDbIndex());
	m_Rcon->Start();

	m_WorkQueue.SetExceptionCallback(std::bind(&RedisWriter::ExceptionHandler, this, _1));

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(15);
	m_ReconnectTimer->OnTimerExpired.connect(std::bind(&RedisWriter::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	m_SubscriptionTimer = new Timer();
	m_SubscriptionTimer->SetInterval(15);
	m_SubscriptionTimer->OnTimerExpired.connect(std::bind(&RedisWriter::UpdateSubscriptionsTimerHandler, this));
	m_SubscriptionTimer->Start();

	m_StatsTimer = new Timer();
	m_StatsTimer->SetInterval(10);
	m_StatsTimer->OnTimerExpired.connect(std::bind(&RedisWriter::PublishStatsTimerHandler, this));
	m_StatsTimer->Start();

	m_WorkQueue.SetName("RedisWriter");

	boost::thread thread(std::bind(&RedisWriter::HandleEvents, this));
	thread.detach();

}

void RedisWriter::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "RedisWriter", "Exception during redis query. Verify that Redis is operational.");

	Log(LogDebug, "RedisWriter")
		<< "Exception during redis operation: " << DiagnosticInformation(exp);
}

void RedisWriter::ReconnectTimerHandler()
{
	m_WorkQueue.Enqueue(std::bind(&RedisWriter::TryToReconnect, this));
}

void RedisWriter::TryToReconnect()
{
	AssertOnWorkQueue();

	if (m_ConfigDumpDone)
		return;
	else
		m_Rcon->Start();

	if (!m_Rcon->IsConnected())
		return;

	UpdateSubscriptions();

	if (m_ConfigDumpInProgress || m_ConfigDumpDone)
		return;

	/* Config dump */
	m_ConfigDumpInProgress = true;

	UpdateAllConfigObjects();

	m_ConfigDumpDone = true;

	m_ConfigDumpInProgress = false;
}

void RedisWriter::UpdateSubscriptionsTimerHandler()
{
	m_WorkQueue.Enqueue(std::bind(&RedisWriter::UpdateSubscriptions, this));
}

void RedisWriter::UpdateSubscriptions()
{
	AssertOnWorkQueue();

	Log(LogInformation, "RedisWriter", "Updating Redis subscriptions");

	/* TODO:
	 * Silently return in this case. Usually the RedisConnection checks for connectivity and logs in failure case.
	 * But since we expect and answer here and break Icinga in case of receiving no answer/an unexpected one we opt for
	 * better safe than sorry here. Future implementation needs to include an improved error handling and answer verification.
	 */
	if (!m_Rcon->IsConnected())
		return;

	long long cursor = 0;

	String keyPrefix = "icinga:subscription:";

	do {
		auto reply = RedisGet({ "SCAN", Convert::ToString(cursor), "MATCH", keyPrefix + "*", "COUNT", "1000" });

		VERIFY(reply->type == REDIS_REPLY_ARRAY);
		VERIFY(reply->elements % 2 == 0);

		redisReply *cursorReply = reply->element[0];
		cursor = Convert::ToLong(cursorReply->str);

		redisReply *keysReply = reply->element[1];

		for (size_t i = 0; i < keysReply->elements; i++) {
			if (boost::algorithm::ends_with(keysReply->element[i]->str, ":limit"))
				continue;
			redisReply *keyReply = keysReply->element[i];
			VERIFY(keyReply->type == REDIS_REPLY_STRING);

			RedisSubscriptionInfo rsi;
			String key = keysReply->element[i]->str;

			if (!RedisWriter::GetSubscriptionTypes(key, rsi)) {
				Log(LogInformation, "RedisWriter")
					<< "Subscription \"" << key << "\" has no types listed.";
			} else {
				m_Subscriptions[key.SubStr(keyPrefix.GetLength())] = rsi;
			}
		}
	} while (cursor != 0);

	Log(LogInformation, "RedisWriter")
		<< "Current Redis event subscriptions: " << m_Subscriptions.size();
}

bool RedisWriter::GetSubscriptionTypes(String key, RedisSubscriptionInfo& rsi)
{
	try {
		redisReply *redisReply = RedisGet({ "SMEMBERS", key });
		VERIFY(redisReply->type == REDIS_REPLY_ARRAY);

		if (redisReply->elements == 0)
			return false;

		for (size_t j = 0; j < redisReply->elements; j++) {
			rsi.EventTypes.insert(redisReply->element[j]->str);
		}

		Log(LogInformation, "RedisWriter")
			<< "Subscriber Info - Key: " << key << " Value: " << Value(Array::FromSet(rsi.EventTypes));

	} catch (const std::exception& ex) {
		Log(LogWarning, "RedisWriter")
			<< "Invalid Redis subscriber info for subscriber '" << key << "': " << DiagnosticInformation(ex);

		return false;
	}

	return true;
}

void RedisWriter::PublishStatsTimerHandler(void)
{
	m_WorkQueue.Enqueue(std::bind(&RedisWriter::PublishStats, this));
}

void RedisWriter::PublishStats()
{
	AssertOnWorkQueue();

	if (!m_Rcon->IsConnected())
		return;

	Dictionary::Ptr status = GetStats();
	String jsonStats = JsonEncode(status);

	m_Rcon->ExecuteQuery({ "PUBLISH", "icinga:stats", jsonStats });
}

void RedisWriter::HandleEvents()
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

		m_WorkQueue.Enqueue(std::bind(&RedisWriter::SendEvent, this, event));
	}

	queue->RemoveClient(this);
	EventQueue::UnregisterIfUnused(queueName, queue);
}

void RedisWriter::HandleEvent(const Dictionary::Ptr& event)
{
	AssertOnWorkQueue();

	for (const std::pair<String, RedisSubscriptionInfo>& kv : m_Subscriptions) {
		const auto& name = kv.first;
		const auto& rsi = kv.second;

		if (rsi.EventTypes.find(event->Get("type")) == rsi.EventTypes.end())
			continue;

		String body = JsonEncode(event);

		redisReply *maxExists = RedisGet({ "EXISTS", "icinga:subscription:" + name + ":limit" });

		long maxEvents = MAX_EVENTS_DEFAULT;
		if (maxExists->integer) {
			redisReply *redisReply = RedisGet({ "GET", "icinga:subscription:" + name + ":limit"});
			VERIFY(redisReply->type == REDIS_REPLY_STRING);

			Log(LogInformation, "RedisWriter")
				<< "Got limit " << redisReply->str << " for " << name;

			maxEvents = Convert::ToLong(redisReply->str);
		}

		m_Rcon->ExecuteQueries({
			{ "MULTI" },
			{ "LPUSH", "icinga:event:" + name, body },
			{ "LTRIM", "icinga:event:" + name, "0", String(maxEvents - 1)},
			{ "EXEC" }});
	}
}

void RedisWriter::SendEvent(const Dictionary::Ptr& event)
{
	AssertOnWorkQueue();

	if (!m_Rcon->IsConnected())
		return;

	String type = event->Get("type");
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
					}
				}
			}
			event->Set("comment_id", GetObjectIdentifier(AckComment));
		}
	}

	String body = JsonEncode(event);

//	Log(LogInformation, "RedisWriter")
//		<< "Sending event \"" << body << "\"";

	m_Rcon->ExecuteQueries({
		{ "PUBLISH", "icinga:event:all", body },
		{ "PUBLISH", "icinga:event:" + event->Get("type"), body }});
}

void RedisWriter::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "RedisWriter")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<RedisWriter>::Stop(runtimeRemoved);
}

void RedisWriter::AssertOnWorkQueue()
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}


/*
 * This whole spiel is required as we mostly use a "fire and forget" approach with the Redis Connection. To wait for a
 * reply from Redis we have to wait for the callback to finish, this is done with the help of this struct. ready, cv
 * and mtx are used for making sure we have the redisReply when we return.
 */
struct synchronousWait {
	bool ready;
	boost::condition_variable cv;
	boost::mutex mtx;
	redisReply* reply;
};

void RedisWriter::RedisQueryCallback(redisAsyncContext *c, void *r, void *p) {
	auto wait = (struct synchronousWait*) p;
	auto rp = reinterpret_cast<redisReply *>(r);


	if (r == NULL)
		wait->reply = nullptr;
	else
		wait->reply = RedisWriter::dupReplyObject(rp);

	boost::mutex::scoped_lock lock(wait->mtx);
	wait->ready = true;
	wait->cv.notify_all();
}


redisReply* RedisWriter::RedisGet(const std::vector<String>& query) {
	auto *wait = new synchronousWait;
	wait->ready = false;

	m_Rcon->ExecuteQuery(query, RedisQueryCallback, wait);

	boost::mutex::scoped_lock lock(wait->mtx);
	while (!wait->ready) {
		wait->cv.wait(lock);
		if (!wait->ready)
			wait->ready = true;
	}

	return wait->reply;
}