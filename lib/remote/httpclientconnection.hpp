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

#ifndef HTTPCLIENTCONNECTION_H
#define HTTPCLIENTCONNECTION_H

#include "remote/httprequest.hpp"
#include "remote/httpresponse.hpp"
#include "base/stream.hpp"
#include "base/timer.hpp"
#include <deque>

namespace icinga
{

/**
 * An HTTP client connection.
 *
 * @ingroup remote
 */
class I2_REMOTE_API HttpClientConnection : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(HttpClientConnection);

	HttpClientConnection(const String& host, const String& port, bool tls = true);

	void Start(void);

	Stream::Ptr GetStream(void) const;
	String GetHost(void) const;
	String GetPort(void) const;
	bool GetTls(void) const;

	void Disconnect(void);

	std::shared_ptr<HttpRequest> NewRequest(void);

	typedef std::function<void(HttpRequest&, HttpResponse&)> HttpCompletionCallback;
	void SubmitRequest(const std::shared_ptr<HttpRequest>& request, const HttpCompletionCallback& callback);

private:
	String m_Host;
	String m_Port;
	bool m_Tls;
	Stream::Ptr m_Stream;
	std::deque<std::pair<std::shared_ptr<HttpRequest>, HttpCompletionCallback> > m_Requests;
	std::shared_ptr<HttpResponse> m_CurrentResponse;
	boost::mutex m_DataHandlerMutex;

	StreamReadContext m_Context;

	void Reconnect(void);
	bool ProcessMessage(void);
	void DataAvailableHandler(const Stream::Ptr& stream);

	void ProcessMessageAsync(HttpRequest& request);
};

}

#endif /* HTTPCLIENTCONNECTION_H */
