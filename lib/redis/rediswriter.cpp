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
#include "remote/eventqueue.hpp"
#include "base/json.hpp"
#include <boost/algorithm/string.hpp>

using namespace icinga;

//TODO Make configurable and figure out a sane default
#define MAX_EVENTS_DEFAULT 5000

REGISTER_TYPE(RedisWriter);

RedisWriter::RedisWriter()
	: m_Context(NULL)
{
	m_WorkQueue.SetName("RedisWriter");
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

	if (m_Context) {
		redisFree(m_Context);
		m_Context = NULL;
	}
}

void RedisWriter::ReconnectTimerHandler()
{
	m_WorkQueue.Enqueue(std::bind(&RedisWriter::TryToReconnect, this));
}

void RedisWriter::TryToReconnect()
{
	AssertOnWorkQueue();

	if (m_Context)
		return;

	String path = GetPath();
	String host = GetHost();

	Log(LogInformation, "RedisWriter", "Trying to connect to redis server");

	if (path.IsEmpty())
		m_Context = redisConnect(host.CStr(), GetPort());
	else
		m_Context = redisConnectUnix(path.CStr());

	if (!m_Context || m_Context->err) {
		if (!m_Context) {
			Log(LogWarning, "RedisWriter", "Cannot allocate redis context.");
		} else {
			Log(LogWarning, "RedisWriter", "Connection error: ")
				<< m_Context->errstr;
		}

		if (m_Context) {
			redisFree(m_Context);
			m_Context = NULL;
		}

		return;
	}

	String password = GetPassword();

	/* TODO: exception is fired but terminates reconnect silently.
	 * Error case: Password does not match, or even: "Client sent AUTH, but no password is set" which also results in an error.
	 */
	if (!password.IsEmpty())
		ExecuteQuery({ "AUTH", password });

	int dbIndex = GetDbIndex();

	if (dbIndex != 0)
		ExecuteQuery({ "SELECT", Convert::ToString(dbIndex) });

	UpdateSubscriptions();

	/* Config dump */
	m_ConfigDumpInProgress = true;

	UpdateAllConfigObjects();

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

	m_Subscriptions.clear();

	if (!m_Context)
		return;

	long long cursor = 0;

	String keyPrefix = "icinga:subscription:";

	do {
		std::shared_ptr<redisReply> reply = ExecuteQuery({ "SCAN", Convert::ToString(cursor), "MATCH", keyPrefix + "*", "COUNT", "1000" });

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
		std::shared_ptr<redisReply> redisReply = ExecuteQuery({ "SMEMBERS", key });
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

	if (!m_Context)
		return;

	Dictionary::Ptr status = GetStats();
	String jsonStats = JsonEncode(status);

	ExecuteQuery({ "PUBLISH", "icinga:stats", jsonStats });
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

	if (!m_Context)
		return;

	for (const std::pair<String, RedisSubscriptionInfo>& kv : m_Subscriptions) {
		const auto& name = kv.first;
		const auto& rsi = kv.second;

		if (rsi.EventTypes.find(event->Get("type")) == rsi.EventTypes.end())
			continue;

		String body = JsonEncode(event);

		std::shared_ptr<redisReply> maxExists = ExecuteQuery({ "EXISTS", "icinga:subscription:" + name + ":limit" });
		long maxEvents = MAX_EVENTS_DEFAULT;
		if (maxExists->integer) {
			std::shared_ptr<redisReply> redisReply = ExecuteQuery({ "GET", "icinga:subscription:" + name + ":limit"});
			VERIFY(redisReply->type == REDIS_REPLY_STRING);

			Log(LogInformation, "RedisWriter")
				<< "Got limit " << redisReply->str << " for " << name;

			maxEvents = Convert::ToLong(redisReply->str);
		}

		ExecuteQuery({ "MULTI" });
		ExecuteQuery({ "LPUSH", "icinga:event:" + name, body });
		ExecuteQuery({ "LTRIM", "icinga:event:" + name, "0", String(maxEvents - 1)});
		ExecuteQuery({ "EXEC" });
	}
}

void RedisWriter::SendEvent(const Dictionary::Ptr& event)
{
	AssertOnWorkQueue();

	if (!m_Context)
		return;

	String body = JsonEncode(event);

//	Log(LogInformation, "RedisWriter")
//		<< "Sending event \"" << body << "\"";

	ExecuteQuery({ "PUBLISH", "icinga:event:all", body });
	ExecuteQuery({ "PUBLISH", "icinga:event:" + event->Get("type"), body });
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

std::shared_ptr<redisReply> RedisWriter::ExecuteQuery(const std::vector<String>& query)
{
	const char **argv;
	size_t *argvlen;

	argv = new const char *[query.size()];
	argvlen = new size_t[query.size()];

	for (std::vector<String>::size_type i = 0; i < query.size(); i++) {
		argv[i] = query[i].CStr();
		argvlen[i] = query[i].GetLength();
	}

	redisReply *reply = reinterpret_cast<redisReply *>(redisCommandArgv(m_Context, query.size(), argv, argvlen));

	delete [] argv;
	delete [] argvlen;

	if (reply->type == REDIS_REPLY_ERROR) {
		Log(LogCritical, "RedisWriter")
			<< "Redis query failed: " << reply->str;

		String msg = reply->str;

		freeReplyObject(reply);

		BOOST_THROW_EXCEPTION(
			redis_error()
				<< errinfo_message(msg)
				<< errinfo_redis_query(Utility::Join(Array::FromVector(query), ' ', false))
		);
	}

	return std::shared_ptr<redisReply>(reply, freeReplyObject);
}

std::vector<std::shared_ptr<redisReply> > RedisWriter::ExecuteQueries(const std::vector<std::vector<String> >& queries)
{
	const char **argv;
	size_t *argvlen;

	for (const auto& query : queries) {
		argv = new const char *[query.size()];
		argvlen = new size_t[query.size()];

		for (std::vector<String>::size_type i = 0; i < query.size(); i++) {
			argv[i] = query[i].CStr();
			argvlen[i] = query[i].GetLength();
		}

		redisAppendCommandArgv(m_Context, query.size(), argv, argvlen);

		delete [] argv;
		delete [] argvlen;
	}

	std::vector<std::shared_ptr<redisReply> > replies;

	for (size_t i = 0; i < queries.size(); i++) {
		redisReply *rawReply;

		if (redisGetReply(m_Context, reinterpret_cast<void **>(&rawReply)) == REDIS_ERR) {
			BOOST_THROW_EXCEPTION(
			    redis_error()
				<< errinfo_message("redisGetReply() failed")
			);
		}

		std::shared_ptr<redisReply> reply(rawReply, freeReplyObject);
		replies.push_back(reply);
	}

	for (size_t i = 0; i < queries.size(); i++) {
		const auto& query = queries[i];
		const auto& reply = replies[i];

		if (reply->type == REDIS_REPLY_ERROR) {
			Log(LogCritical, "RedisWriter")
				<< "Redis query failed: " << reply->str;

			String msg = reply->str;

			BOOST_THROW_EXCEPTION(
				redis_error()
					<< errinfo_message(msg)
					<< errinfo_redis_query(Utility::Join(Array::FromVector(query), ' ', false))
			);
		}
	}

	return replies;
}
