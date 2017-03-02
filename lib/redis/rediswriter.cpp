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

/**
 * Starts the component.
 */
void RedisWriter::Start(bool runtimeCreated)
{
	ObjectImpl<RedisWriter>::Start(runtimeCreated);

	Log(LogInformation, "RedisWriter")
	    << "'" << GetName() << "' started.";

	boost::thread thread(boost::bind(&RedisWriter::ConnectionThreadProc, this));
	thread.detach();
}

void RedisWriter::ConnectionThreadProc(void)
{
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
	}

	for (;;) {
		String password = GetPassword();

		if (!password.IsEmpty()) {
			redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(m_Context, "AUTH %s", password.CStr()));

			//TODO: Verify if we can continue here.
			if (!reply)
				continue;

			if (reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR) {
				Log(LogInformation, "RedisWriter")
				    << "AUTH: " << reply->str;
			}

			freeReplyObject(reply);
		}

		HandleEvents();

		for (;;) {
			Log(LogInformation, "RedisWriter", "Trying to reconnect to redis server");

			if (redisReconnect(m_Context) == REDIS_OK) {
				Log(LogInformation, "RedisWriter", "Connection to redis server was reestablished");
				break;
			}

			Log(LogInformation, "RedisWriter", "Unable to reconnect to redis server: Waiting for next attempt");

			Utility::Sleep(15);
		}
	}
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
		Dictionary::Ptr result = queue->WaitForEvent(this);

		if (!result)
			continue;

		String body = JsonEncode(result);

		redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(m_Context, "LPUSH icinga:events %s", body.CStr()));

		if (!reply)
			break;

		if (reply->type == REDIS_REPLY_STATUS || reply->type == REDIS_REPLY_ERROR) {
			Log(LogInformation, "RedisWriter")
			    << "LPUSH icinga:events: " << reply->str;
		}

		if (reply->type == REDIS_REPLY_ERROR) {
			freeReplyObject(reply);
			break;
		}

		freeReplyObject(reply);
	}

	queue->RemoveClient(this);
	EventQueue::UnregisterIfUnused(queueName, queue);
}

void RedisWriter::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "RedisWriter")
	    << "'" << GetName() << "' stopped.";

	ObjectImpl<RedisWriter>::Stop(runtimeRemoved);
}
