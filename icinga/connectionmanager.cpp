#include "i2-icinga.h"

using namespace icinga;

void ConnectionManager::SetIdentity(string identity)
{
	m_Identity = identity;
}

string ConnectionManager::GetIdentity(void)
{
	return m_Identity;
}

void ConnectionManager::AddListener(unsigned short port)
{
	JsonRpcServer::Ptr server = make_shared<JsonRpcServer>();
	RegisterServer(server);

	server->MakeSocket();
	server->Bind(port);
	server->Listen();
	server->Start();
}

void ConnectionManager::AddConnection(string host, short port)
{
	JsonRpcClient::Ptr client = make_shared<JsonRpcClient>();
	RegisterClient(client);

	client->MakeSocket();
	client->Connect(host, port);
	client->Start();
}

void ConnectionManager::RegisterServer(JsonRpcServer::Ptr server)
{
	m_Servers.push_front(server);
	server->OnNewClient += bind_weak(&ConnectionManager::NewClientHandler, shared_from_this());
}

void ConnectionManager::UnregisterServer(JsonRpcServer::Ptr server)
{
	m_Servers.remove(server);
	// TODO: unbind event
}

void ConnectionManager::RegisterClient(JsonRpcClient::Ptr client)
{
	m_Clients.push_front(client);
	client->OnNewMessage += bind_weak(&ConnectionManager::NewMessageHandler, shared_from_this());
	client->OnClosed += bind_weak(&ConnectionManager::CloseClientHandler, shared_from_this());
	client->OnError += bind_weak(&ConnectionManager::ErrorClientHandler, shared_from_this());
}

void ConnectionManager::UnregisterClient(JsonRpcClient::Ptr client)
{
	m_Clients.remove(client);
	// TODO: unbind event
}

int ConnectionManager::NewClientHandler(NewClientEventArgs::Ptr ncea)
{
	JsonRpcClient::Ptr client = static_pointer_cast<JsonRpcClient>(ncea->Client);
	RegisterClient(client);

	return 0;
}

int ConnectionManager::CloseClientHandler(EventArgs::Ptr ea)
{
	JsonRpcClient::Ptr client = static_pointer_cast<JsonRpcClient>(ea->Source);
	UnregisterClient(client);

	if (client->GetPeerHost() != string()) {
		Timer::Ptr timer = make_shared<Timer>();
		timer->SetInterval(30);
		timer->SetUserArgs(ea);
		timer->OnTimerExpired += bind_weak(&ConnectionManager::ReconnectClientHandler, shared_from_this());
		timer->Start();
		m_ReconnectTimers.push_front(timer);
	}

	return 0;
}

int ConnectionManager::ErrorClientHandler(SocketErrorEventArgs::Ptr ea)
{
	cout << "Error occured for JSON-RPC socket: Code=" << ea->Code << "; Message=" << ea->Message << endl;

	return 0;
}

int ConnectionManager::ReconnectClientHandler(TimerEventArgs::Ptr ea)
{
	JsonRpcClient::Ptr client = static_pointer_cast<JsonRpcClient>(ea->UserArgs->Source);
	Timer::Ptr timer = static_pointer_cast<Timer>(ea->Source);

	AddConnection(client->GetPeerHost(), client->GetPeerPort());

	timer->Stop();
	m_ReconnectTimers.remove(timer);

	return 0;
}

int ConnectionManager::NewMessageHandler(NewMessageEventArgs::Ptr nmea)
{
	JsonRpcMessage::Ptr request = nmea->Message;
	JsonRpcClient::Ptr client = static_pointer_cast<JsonRpcClient>(nmea->Source);

	map<string, event<NewMessageEventArgs::Ptr> >::iterator i;
	i = m_Methods.find(request->GetMethod());

	if (i == m_Methods.end()) {
		JsonRpcMessage::Ptr response = make_shared<JsonRpcMessage>();
		response->SetVersion("2.0");
		response->SetError("Unknown method.");
		response->SetID(request->GetID());
		Netstring::WriteJSONToFIFO(client->GetSendQueue(), response->GetJSON());

		return 0;
	}

	i->second(nmea);

	return 0;
}

void ConnectionManager::RegisterMethod(string method, function<int (NewMessageEventArgs::Ptr)> callback)
{
	m_Methods[method] += callback;
}

void ConnectionManager::UnregisterMethod(string method, function<int (NewMessageEventArgs::Ptr)> callback)
{
	// TODO: implement
	//m_Methods[method] -= callback;
}

void ConnectionManager::SendMessage(JsonRpcMessage::Ptr message)
{
	/* TODO: filter messages based on event subscriptions; also loopback message to our own handlers */
	for (list<JsonRpcClient::Ptr>::iterator i = m_Clients.begin(); i != m_Clients.end(); i++)
	{
		JsonRpcClient::Ptr client = *i;
		client->SendMessage(message);
	}
}
