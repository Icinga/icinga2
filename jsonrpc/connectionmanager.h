#ifndef I2_CONNECTIONMANAGER_H
#define I2_CONNECTIONMANAGER_H

namespace icinga
{

class ConnectionManager : public Object
{
	list<JsonRpcServer::RefType> m_Servers;
	list<JsonRpcClient::RefType> m_Clients;

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

	event<NewMessageEventArgs::RefType> OnNewMessage;
};

}

#endif /* I2_CONNECTIONMANAGER_H */