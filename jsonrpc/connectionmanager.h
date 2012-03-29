#ifndef I2_CONNECTIONMANAGER_H
#define I2_CONNECTIONMANAGER_H

namespace icinga
{

using std::map;

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

	void BindServer(JsonRpcServer::RefType server);
	void UnbindServer(JsonRpcServer::RefType server);

	void BindClient(JsonRpcClient::RefType client);
	void UnbindClient(JsonRpcClient::RefType client);

	void RegisterMethod(string method, function<int (NewMessageEventArgs::RefType)> function);
	void UnregisterMethod(string method, function<int (NewMessageEventArgs::RefType)> function);
};

}

#endif /* I2_CONNECTIONMANAGER_H */