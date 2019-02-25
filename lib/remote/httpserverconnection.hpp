/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
