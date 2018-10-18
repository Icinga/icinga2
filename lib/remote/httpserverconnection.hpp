/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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
#include "remote/httpresponse.hpp"
#include "remote/apiuser.hpp"
#include "base/tlsstream.hpp"
#include "base/workqueue.hpp"
#include <boost/thread/recursive_mutex.hpp>

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

	void Start();

	ApiUser::Ptr GetApiUser() const;
	bool IsAuthenticated() const;
	TlsStream::Ptr GetStream() const;

	void Disconnect();

private:
	ApiUser::Ptr m_ApiUser;
	ApiUser::Ptr m_AuthenticatedUser;
	TlsStream::Ptr m_Stream;
	double m_Seen;
	HttpRequest m_CurrentRequest;
	boost::recursive_mutex m_DataHandlerMutex;
	WorkQueue m_RequestQueue;
	int m_PendingRequests;
	String m_PeerAddress;

	StreamReadContext m_Context;

	bool ProcessMessage();
	void DataAvailableHandler();

	static void StaticInitialize();
	static void TimeoutTimerHandler();
	void CheckLiveness();

	bool ManageHeaders(HttpResponse& response);

	void ProcessMessageAsync(HttpRequest& request, HttpResponse& response, const ApiUser::Ptr&);
};

}

#endif /* HTTPSERVERCONNECTION_H */
