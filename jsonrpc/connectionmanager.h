#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

namespace icinga
{

class ConnectionManager : public Object
{
	list<JsonRpcServer::RefType> m_Servers;
	list<JsonRpcClient::RefType> m_Clients;
	map< string, event<NewMessageEventArgs::RefType> > m_Methods;

	int NewClientHandler(NewClientEventArgs::RefType ncea);
	int CloseClientHandler(EventArgs::RefType ea);
	int NewMessageHandler(NewMessageEventArgs::RefType nmea);

public:
	typedef shared_ptr<ConnectionManager> RefType;
	typedef weak_ptr<ConnectionManager> WeakRefType;

	void RegisterServer(JsonRpcServer::RefType server);
	void UnregisterServer(JsonRpcServer::RefType server);

	void RegisterClient(JsonRpcClient::RefType client);
	void UnregisterClient(JsonRpcClient::RefType client);

	void RegisterMethod(string method, function<int (NewMessageEventArgs::RefType)> function);
	void UnregisterMethod(string method, function<int (NewMessageEventArgs::RefType)> function);

	void SendMessage(JsonRpcMessage::RefType message);
};

}

#endif /* CONNECTIONMANAGER_H */
