/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
class HttpClientConnection final : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(HttpClientConnection);

	HttpClientConnection(String host, String port, bool tls = true);

	void Start();

	Stream::Ptr GetStream() const;
	String GetHost() const;
	String GetPort() const;
	bool GetTls() const;

	void Disconnect();

	std::shared_ptr<HttpRequest> NewRequest();

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

	void Reconnect();
	bool ProcessMessage();
	void DataAvailableHandler(const Stream::Ptr& stream);

	void ProcessMessageAsync(HttpRequest& request);
};

}

#endif /* HTTPCLIENTCONNECTION_H */
