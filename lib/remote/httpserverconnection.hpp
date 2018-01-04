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

#ifndef HTTPSERVERCONNECTION_H
#define HTTPSERVERCONNECTION_H

#include "remote/httprequest.hpp"
#include "remote/apiuser.hpp"
#include "base/tlsstream.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"

namespace icinga
{

/**
 * An API client connection.
 *
 * @ingroup remote
 */
class HttpServerConnection final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(HttpServerConnection);

	HttpServerConnection(const String& identity, bool authenticated, const TlsStream::Ptr& stream);

	void Start(void);

	ApiUser::Ptr GetApiUser(void) const;
	bool IsAuthenticated(void) const;
	TlsStream::Ptr GetStream(void) const;

	void Disconnect(void);

private:
	ApiUser::Ptr m_ApiUser;
	TlsStream::Ptr m_Stream;
	double m_Seen;
	HttpRequest m_CurrentRequest;
	boost::mutex m_DataHandlerMutex;
	WorkQueue m_RequestQueue;
	int m_PendingRequests;

	StreamReadContext m_Context;

	bool ProcessMessage(void);
	void DataAvailableHandler(void);

	static void StaticInitialize(void);
	static void TimeoutTimerHandler(void);
	void CheckLiveness(void);

	void ProcessMessageAsync(HttpRequest& request);
};

}

#endif /* HTTPSERVERCONNECTION_H */
