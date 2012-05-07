#ifndef ENDPOINTMANAGER_H
#define ENDPOINTMANAGER_H

namespace icinga
{

struct I2_ICINGA_API NewEndpointEventArgs : public EventArgs
{
	icinga::Endpoint::Ptr Endpoint;
};

class I2_ICINGA_API EndpointManager : public Object
{
	string m_Identity;
	shared_ptr<SSL_CTX> m_SSLContext;

	list<JsonRpcServer::Ptr> m_Servers;
	list<Endpoint::Ptr> m_Endpoints;

	void RegisterServer(JsonRpcServer::Ptr server);
	void UnregisterServer(JsonRpcServer::Ptr server);

	int NewClientHandler(const NewClientEventArgs& ncea);

public:
	typedef shared_ptr<EndpointManager> Ptr;
	typedef weak_ptr<EndpointManager> WeakPtr;

	void SetIdentity(string identity);
	string GetIdentity(void) const;

	void SetSSLContext(shared_ptr<SSL_CTX> sslContext);
	shared_ptr<SSL_CTX> GetSSLContext(void) const;

	void AddListener(string service);
	void AddConnection(string node, string service);

	void RegisterEndpoint(Endpoint::Ptr endpoint);
	void UnregisterEndpoint(Endpoint::Ptr endpoint);

	void SendUnicastRequest(Endpoint::Ptr sender, Endpoint::Ptr recipient, const JsonRpcRequest& request, bool fromLocal = true);
	void SendAnycastRequest(Endpoint::Ptr sender, const JsonRpcRequest& request, bool fromLocal = true);
	void SendMulticastRequest(Endpoint::Ptr sender, const JsonRpcRequest& request, bool fromLocal = true);

	void ForeachEndpoint(function<int (const NewEndpointEventArgs&)> callback);

	bool HasConnectedEndpoint(string identity) const;

	Event<NewEndpointEventArgs> OnNewEndpoint;
};

}

#endif /* ENDPOINTMANAGER_H */
