#ifndef ENDPOINTMANAGER_H
#define ENDPOINTMANAGER_H

namespace icinga
{

class I2_ICINGA_API EndpointManager : public Object
{
	list<JsonRpcServer::Ptr> m_Servers;
	list<JsonRpcClient::Ptr> m_Clients;
	list<Timer::Ptr> m_ReconnectTimers;
	list<Endpoint::Ptr> m_Endpoints;
	string m_Identity;

	int NewClientHandler(NewClientEventArgs::Ptr ncea);
	int CloseClientHandler(EventArgs::Ptr ea);
	int ErrorClientHandler(SocketErrorEventArgs::Ptr ea);
	int ReconnectClientHandler(TimerEventArgs::Ptr ea);
	int NewMessageHandler(NewMessageEventArgs::Ptr nmea);

	void RegisterClient(JsonRpcClient::Ptr server);
	void UnregisterClient(JsonRpcClient::Ptr server);

	void RegisterServer(JsonRpcServer::Ptr server);
	void UnregisterServer(JsonRpcServer::Ptr server);

public:
	typedef shared_ptr<EndpointManager> Ptr;
	typedef weak_ptr<EndpointManager> WeakPtr;

	void SetIdentity(string identity);
	string GetIdentity(void) const;

	void AddListener(unsigned short port);
	void AddConnection(string host, short port);

	void RegisterEndpoint(Endpoint::Ptr endpoint);
	void UnregisterEndpoint(Endpoint::Ptr endpoint);

	void SendMessage(Endpoint::Ptr source, Endpoint::Ptr destination, JsonRpcMessage::Ptr message);
};

}

#endif /* ENDPOINTMANAGER_H */
