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

	boost::thread thread(boost::bind(&RedisWriter::HandleEvents, this));
	thread.detach();

	String path = GetPath();
	String host = GetHost();

	if (path.IsEmpty())
		m_Context = redisConnect(host.CStr(), GetPort());
	else
		m_Context = redisConnectUnix(path.CStr());

	String password = GetPassword();

	void *reply = redisCommand(m_Context, "AUTH %s", password.CStr());
	freeReplyObject(reply);
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

		//TODO: Reconnect handling
		try {
			void *reply = redisCommand(m_Context, "LPUSH icinga:events %s", body.CStr());
			freeReplyObject(reply);
		} catch (const std::exception&) {
			queue->RemoveClient(this);
			EventQueue::UnregisterIfUnused(queueName, queue);
			throw;
		}
	}
}
