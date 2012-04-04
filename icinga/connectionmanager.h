#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

namespace icinga
{

class ConnectionManager : public Object
{
	list<JsonRpcServer::Ptr> m_Servers;
	list<JsonRpcClient::Ptr> m_Clients;
	map< string, event<NewMessageEventArgs::Ptr> > m_Methods;
	list<Timer::Ptr> m_ReconnectTimers;
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
	typedef shared_ptr<ConnectionManager> Ptr;
	typedef weak_ptr<ConnectionManager> WeakPtr;

	void SetIdentity(string identity);
	string GetIdentity(void);

	void AddListener(unsigned short port);
	void AddConnection(string host, short port);

	void RegisterMethod(string method, function<int (NewMessageEventArgs::Ptr)> callback);
	void UnregisterMethod(string method, function<int (NewMessageEventArgs::Ptr)> callback);

	void SendMessage(JsonRpcMessage::Ptr message);
};

}

#endif /* CONNECTIONMANAGER_H */
