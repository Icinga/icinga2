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

namespace icinga {
/**
 * An Async Redis connection.
 *
 * @ingroup redis
 */
class RedisConnection final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(RedisConnection);

	RedisConnection(const String host, const int port, const String path);

	void Start();

	void Connect();

	void ExecuteQuery(const std::vector<String>& query, redisCallbackFn *fn = nullptr, void *privdata = nullptr);
	void ExecuteQueries(const std::vector<std::vector<String> >& queries, redisCallbackFn *fn = nullptr, void *privdata = nullptr);


private:
	static void StaticInitialize();
	void SendMessageInternal(const std::vector<String>& query, redisCallbackFn *fn, void *privdata);
	void AssertOnWorkQueue();
	static void DisconnectCallback(const redisAsyncContext *c, int status);

	WorkQueue m_RedisConnectionWorkQueue{100000};

	redisAsyncContext *m_Context;

	String m_Path;
	String m_Host;
	int m_Port;
};

}

#endif //REDISCONNECTION_H
