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
#include <hiredis/hiredis.h>
#include <base/logger.hpp>
#include "base/utility.hpp"

using namespace icinga;
/*
struct redis_error : virtual std::exception, virtual boost::exception { };
struct errinfo_redis_query_;
typedef boost::error_info<struct errinfo_redis_query_, std::string> errinfo_redis_query;
*/
RedisConnection::RedisConnection(const String host, const int port, const String path) :
m_Host(host), m_Port(port), m_Path(path) {
	m_RedisConnectionWorkQueue.SetName("RedisConnection");
}

void RedisConnection::StaticInitialize()
{

}

void RedisConnection::Start()
{
	RedisConnection::Connect();
}

void RedisConnection::AssertOnWorkQueue()
{
	ASSERT(m_RedisConnectionWorkQueue.IsWorkerThread());
}

void RedisConnection::Connect() {
	if (m_Context)
		return;

	Log(LogInformation, "RedisWriter", "Trying to connect to redis server Async");

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

	//TODO: Authentication, DB selection, error handling
}

void RedisConnection::DisconnectCallback(const redisAsyncContext *c, int status) {
	if (status == REDIS_OK)
		Log(LogCritical, "RedisWriter") << "Redis disconnected by user";
	else
		Log(LogCritical, "Rediswriter") << "Redis disconnected for reasons";

}
void RedisConnection::ExecuteQuery(const std::vector<String>& query, redisCallbackFn *fn, void *privdata)
{
	m_RedisConnectionWorkQueue.Enqueue(std::bind(&RedisConnection::SendMessageInternal, this, query, fn, privdata));
}

void RedisConnection::ExecuteQueries(const std::vector<std::vector<String> >& queries, redisCallbackFn *fn, void *privdata)
{
	for (const auto& query : queries) {
		m_RedisConnectionWorkQueue.Enqueue(std::bind(&RedisConnection::SendMessageInternal, this, query, fn, privdata));
	}
}

void RedisConnection::SendMessageInternal(const std::vector<String>& query, redisCallbackFn *fn, void *privdata)
{
	AssertOnWorkQueue();

	if (!m_Context) {
		Log(LogCritical, "RedisWriter")
				<< "Connection lost";
		return;
	}

	const char **argv;
	size_t *argvlen;

	argv = new const char *[query.size()];
	argvlen = new size_t[query.size()];

	for (std::vector<String>::size_type i = 0; i < query.size(); i++) {
		argv[i] = query[i].CStr();
		argvlen[i] = query[i].GetLength();
	}

	int r = redisAsyncCommandArgv(m_Context, fn, privdata, query.size(), argv, argvlen);

	delete [] argv;
	delete [] argvlen;

	if (r == REDIS_REPLY_ERROR) {
		Log(LogCritical, "RedisWriter")
				<< "Redis Async query failed";

		BOOST_THROW_EXCEPTION(
				redis_error()
						<< errinfo_redis_query("FUCK")
						<< errinfo_redis_query(Utility::Join(Array::FromVector(query), ' ', false))
		);
	}
}