#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

namespace icinga
{

class ConnectionManager : public Object
{
	list<JsonRpcServer::Ptr> m_Servers;
	list<JsonRpcClient::Ptr> m_Clients;
	map< string, event<NewMessageEventArgs::Ptr> > m_Methods;

	int NewClientHandler(NewClientEventArgs::Ptr ncea);
	int CloseClientHandler(EventArgs::Ptr ea);
	int NewMessageHandler(NewMessageEventArgs::Ptr nmea);

public:
	typedef shared_ptr<ConnectionManager> Ptr;
	typedef weak_ptr<ConnectionManager> WeakPtr;

	void RegisterServer(JsonRpcServer::Ptr server);
	void UnregisterServer(JsonRpcServer::Ptr server);

	void RegisterClient(JsonRpcClient::Ptr client);
	void UnregisterClient(JsonRpcClient::Ptr client);

	void RegisterMethod(string method, function<int (NewMessageEventArgs::Ptr)> callback);
	void UnregisterMethod(string method, function<int (NewMessageEventArgs::Ptr)> callback);

	void SendMessage(JsonRpcMessage::Ptr message);
};

}

#endif /* CONNECTIONMANAGER_H */
