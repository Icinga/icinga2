/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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
#include "redis/rediswriter.tcpp"
#include "remote/eventqueue.hpp"
#include "base/json.hpp"

using namespace icinga;

REGISTER_TYPE(RedisWriter);

RedisWriter::RedisWriter(void)
    : m_Context(NULL)
{ }

/**
 * Starts the component.
 */
void RedisWriter::Start(bool runtimeCreated)
{
	ObjectImpl<RedisWriter>::Start(runtimeCreated);

	Log(LogInformation, "RedisWriter")
	    << "'" << GetName() << "' started.";

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(15);
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&RedisWriter::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	m_SubscriptionTimer = new Timer();
	m_SubscriptionTimer->SetInterval(15);
	m_SubscriptionTimer->OnTimerExpired.connect(boost::bind(&RedisWriter::UpdateSubscriptionsTimerHandler, this));
	m_SubscriptionTimer->Start();

	boost::thread thread(boost::bind(&RedisWriter::HandleEvents, this));
	thread.detach();
}

void RedisWriter::ReconnectTimerHandler(void)
{
	m_WorkQueue.Enqueue(boost::bind(&RedisWriter::TryToReconnect, this));
}

void RedisWriter::TryToReconnect(void)
{
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

	if (!password.IsEmpty()) {
		redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(m_Context, "AUTH %s", password.CStr()));

		if (!reply) {
			redisFree(m_Context);
			m_Context = NULL;
			return;
		}

		if (reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR) {
			Log(LogInformation, "RedisWriter")
			    << "AUTH: " << reply->str;
		}

		freeReplyObject(reply);
	}

	/* Config dump */
	UpdateAllConfigObjects();
}

void RedisWriter::UpdateSubscriptionsTimerHandler(void)
{
	m_WorkQueue.Enqueue(boost::bind(&RedisWriter::UpdateSubscriptions, this));
}

void RedisWriter::UpdateSubscriptions(void)
{
	if (!m_Context)
		return;

	Log(LogInformation, "RedisWriter", "Updating Redis subscriptions");

	std::map<String, String> subscriptions;
	long long cursor = 0;

	do {
		redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(m_Context, "SCAN %lld MATCH icinga:subscription:* COUNT 1000", cursor));

		if (!reply) {
			redisFree(m_Context);
			m_Context = NULL;
			return;
		}

		if (reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR) {
			Log(LogInformation, "RedisWriter")
			    << "SCAN " << cursor << " MATCH icinga:subscription:* COUNT 1000: " << reply->str;
		}

		VERIFY(reply->type == REDIS_REPLY_ARRAY);
		VERIFY(reply->elements % 2 == 0);

		redisReply *cursorReply = reply->element[0];
		cursor = Convert::ToLong(cursorReply->str);

		redisReply *keysReply = reply->element[1];

		for (size_t i = 0; i < keysReply->elements; i++) {
			redisReply *keyReply = keysReply->element[i];
			VERIFY(keyReply->type == REDIS_REPLY_STRING);

			redisReply *vreply = reinterpret_cast<redisReply *>(redisCommand(m_Context, "GET %s", keyReply->str));

			if (!vreply) {
				freeReplyObject(reply);
				redisFree(m_Context);
				m_Context = NULL;
				return;
			}

			if (vreply->type == REDIS_REPLY_STATUS || vreply->type == REDIS_REPLY_ERROR) {
				Log(LogInformation, "RedisWriter")
				    << "GET " << keyReply->str << ": " << vreply->str;
			}

			subscriptions[keyReply->str] = vreply->str;

			freeReplyObject(vreply);
		}

		freeReplyObject(reply);
	} while (cursor != 0);

	m_Subscriptions.clear();

	for (const std::pair<String, String>& kv : subscriptions) {
		const String& key = kv.first.SubStr(20); /* removes the "icinga:subscription: prefix */
		const String& value = kv.second;

		try {
			Dictionary::Ptr subscriptionInfo = JsonDecode(value);

			Log(LogInformation, "RedisWriter")
			    << "Subscriber Info - Key: " << key << " Value: " << Value(subscriptionInfo);

			RedisSubscriptionInfo rsi;

			Array::Ptr types = subscriptionInfo->Get("types");

			if (types)
				rsi.EventTypes = types->ToSet<String>();

			m_Subscriptions[key] = rsi;
		} catch (const std::exception& ex) {
			Log(LogWarning, "RedisWriter")
			    << "Invalid Redis subscriber info for subscriber '" << key << "': " << DiagnosticInformation(ex);

			continue;
		}
		//TODO
	}

	Log(LogInformation, "RedisWriter")
	    << "Current Redis event subscriptions: " << m_Subscriptions.size();
}

void RedisWriter::HandleEvents(void)
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

		m_WorkQueue.Enqueue(boost::bind(&RedisWriter::HandleEvent, this, event));
	}

	queue->RemoveClient(this);
	EventQueue::UnregisterIfUnused(queueName, queue);
}

void RedisWriter::HandleEvent(const Dictionary::Ptr& event)
{
	if (!m_Context)
		return;

	for (const std::pair<String, RedisSubscriptionInfo>& kv : m_Subscriptions) {
		const auto& name = kv.first;
		const auto& rsi = kv.second;

		if (rsi.EventTypes.find(event->Get("type")) == rsi.EventTypes.end())
			continue;

		String body = JsonEncode(event);

		redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(m_Context, "LPUSH icinga:event:%s %s", name.CStr(), body.CStr()));

		if (!reply) {
			redisFree(m_Context);
			m_Context = NULL;
			return;
		}

		if (reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR) {
			Log(LogInformation, "RedisWriter")
			    << "LPUSH icinga:event:" << kv.first << " " << body << ": " << reply->str;
		}

		if (reply->type == REDIS_REPLY_ERROR) {
			freeReplyObject(reply);
			return;
		}

		freeReplyObject(reply);
	}
}

void RedisWriter::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "RedisWriter")
	    << "'" << GetName() << "' stopped.";

	ObjectImpl<RedisWriter>::Stop(runtimeRemoved);
}
