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

#include "base/object.hpp"
#include "redis/redisconnection.hpp"
#include "base/workqueue.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "redis/rediswriter.hpp"
#include "hiredis/hiredis.h"


using namespace icinga;

RedisConnection::RedisConnection(const String host, const int port, const String path, const String password, const int db) :
		m_Host(host), m_Port(port), m_Path(path), m_Password(password), m_DbIndex(db), m_Context(NULL)
{
	m_RedisConnectionWorkQueue.SetName("RedisConnection");
}

void RedisConnection::StaticInitialize()
{
}

void RedisConnection::Start()
{
	RedisConnection::Connect();

	std::thread thread(std::bind(&RedisConnection::HandleRW, this));
	thread.detach();
}

void RedisConnection::AssertOnWorkQueue()
{
	ASSERT(m_RedisConnectionWorkQueue.IsWorkerThread());
}

void RedisConnection::HandleRW()
{
	Utility::SetThreadName("RedisConnection Handler");

	for (;;) {
		try {
			{
				boost::mutex::scoped_lock lock(m_CMutex);
				redisAsyncHandleWrite(m_Context);
				redisAsyncHandleRead(m_Context);
			}
			Utility::Sleep(0.1);
		} catch (const std::exception&) {
			Log(LogCritical, "RedisWriter", "Internal Redis Error");
		}
	}
}

void RedisConnection::Connect()
{
	if (m_Context)
		return;

	Log(LogInformation, "RedisWriter", "Trying to connect to redis server Async");
	boost::mutex::scoped_lock lock(m_CMutex);

	if (m_Path.IsEmpty())
		m_Context = redisAsyncConnect(m_Host.CStr(), m_Port);
	else
		m_Context = redisAsyncConnectUnix(m_Path.CStr());

	if (!m_Context || m_Context->err) {
		if (!m_Context) {
			Log(LogWarning, "RedisWriter", "Cannot allocate redis context.");
		} else {
			Log(LogWarning, "RedisWriter", "Connection error: ")
					<< m_Context->errstr;
		}

		if (m_Context) {
			redisAsyncFree(m_Context);
			m_Context = NULL;
		}
	}

	redisAsyncSetDisconnectCallback(m_Context, &DisconnectCallback);

	/* TODO: This currently does not work properly:
	 * In case of error the connection is broken, yet the Context is not set to faulty. May be a bug with hiredis.
     * Error case: Password does not match, or even: "Client sent AUTH, but no password is set" which also results in an error.
     */
	if (!m_Password.IsEmpty()) {
		ExecuteQuery({"AUTH", m_Password});
	}

	if (m_DbIndex != 0)
		ExecuteQuery({"SELECT", Convert::ToString(m_DbIndex)});
}

void RedisConnection::Disconnect()
{
	redisAsyncDisconnect(m_Context);
}

void RedisConnection::DisconnectCallback(const redisAsyncContext *c, int status)
{
	if (status == REDIS_OK)
		Log(LogInformation, "RedisWriter") << "Redis disconnected by us";
	else {
		if (c->err != 0)
			Log(LogCritical, "RedisWriter") << "Redis disconnected by server. Reason: " << c->errstr;
		else
			Log(LogCritical, "RedisWriter") << "Redis disconnected by server";
	}

}

bool RedisConnection::IsConnected()
{
	return (REDIS_CONNECTED & m_Context->c.flags) == REDIS_CONNECTED;
}

void RedisConnection::ExecuteQuery(const std::vector<String>& query, redisCallbackFn *fn, void *privdata)
{
	m_RedisConnectionWorkQueue.Enqueue(std::bind(&RedisConnection::SendMessageInternal, this, query, fn, privdata));
}

void
RedisConnection::ExecuteQueries(const std::vector<std::vector<String> >& queries, redisCallbackFn *fn, void *privdata)
{
	for (const auto& query : queries) {
		m_RedisConnectionWorkQueue.Enqueue(std::bind(&RedisConnection::SendMessageInternal, this, query, fn, privdata));
	}
}

void RedisConnection::SendMessageInternal(const std::vector<String>& query, redisCallbackFn *fn, void *privdata)
{
	AssertOnWorkQueue();

	boost::mutex::scoped_lock lock(m_CMutex);

	if (!m_Context) {
		Log(LogCritical, "RedisWriter")
				<< "Connection lost";
		return;
	}

	const char **argv;
	size_t *argvlen;

	argv = new const char *[query.size()];
	argvlen = new size_t[query.size()];
	String debugstr;

	for (std::vector<String>::size_type i = 0; i < query.size(); i++) {
		argv[i] = query[i].CStr();
		argvlen[i] = query[i].GetLength();
		debugstr += argv[i];
		debugstr += " ";
	}

	Log(LogDebug, "RedisWriter, Connection")
		<< "Sending Command: " << debugstr;
	int r = redisAsyncCommandArgv(m_Context, fn, privdata, query.size(), argv, argvlen);

	delete[] argv;
	delete[] argvlen;

	if (r == REDIS_REPLY_ERROR) {
		Log(LogCritical, "RedisWriter")
				<< "Redis Async query failed";

		BOOST_THROW_EXCEPTION(
				redis_error()
						<< errinfo_redis_query(Utility::Join(Array::FromVector(query), ' ', false))
		);
	}
}
