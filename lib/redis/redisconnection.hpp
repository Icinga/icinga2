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

#ifndef REDISCONNECTION_H
#define REDISCONNECTION_H

#include <third-party/hiredis/async.h>
#include "base/object.hpp"
#include "base/workqueue.hpp"

namespace icinga
{
/**
 * An Async Redis connection.
 *
 * @ingroup redis
 */

	enum conn_state{
		Starting,
		Auth,
		DBSelect,
		Done,
	};

	class RedisConnection;
	struct ConnectionState {
		conn_state state;
		RedisConnection *conn;
	};

	class RedisConnection final : public Object
	{
	public:
		DECLARE_PTR_TYPEDEFS(RedisConnection);

		RedisConnection(const String host, const int port, const String path, const String password = "", const int db = 0);

		void Start();

		void Connect();

		void Disconnect();

		bool IsConnected();

		void ExecuteQuery(std::vector<String> query, redisCallbackFn *fn = NULL, void *privdata = NULL);

		void ExecuteQueries(std::vector<std::vector<String>> queries, redisCallbackFn *fn = NULL,
							void *privdata = NULL);

	private:
		static void StaticInitialize();

		void SendMessageInternal(const std::vector<String>& query, redisCallbackFn *fn, void *privdata);
		void SendMessagesInternal(const std::vector<std::vector<String>>& queries, redisCallbackFn *fn, void *privdata);

		void AssertOnWorkQueue();

		void HandleRW();

		static void DisconnectCallback(const redisAsyncContext *c, int status);
		static void ConnectCallback(const redisAsyncContext *c, int status);

		static void RedisInitialCallback(redisAsyncContext *c, void *r, void *p);


		WorkQueue m_RedisConnectionWorkQueue{100000};
		Timer::Ptr m_EventLoop;

		redisAsyncContext *m_Context;

		String m_Path;
		String m_Host;
		int m_Port;
		String m_Password;
		int m_DbIndex;
		bool m_Connected;

		boost::mutex m_CMutex;
		ConnectionState m_State;

	};

	struct redis_error : virtual std::exception, virtual boost::exception { };

	struct errinfo_redis_query_;
	typedef boost::error_info<struct errinfo_redis_query_, std::string> errinfo_redis_query;

}

#endif //REDISCONNECTION_H
