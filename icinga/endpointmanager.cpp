#include "i2-icinga.h"

using namespace icinga;

void EndpointManager::SetIdentity(string identity)
{
	m_Identity = identity;
}

string EndpointManager::GetIdentity(void) const
{
	return m_Identity;
}

void EndpointManager::AddListener(unsigned short port)
{
	JsonRpcServer::Ptr server = make_shared<JsonRpcServer>();
	RegisterServer(server);

	server->MakeSocket();
	server->Bind(port);
	server->Listen();
	server->Start();
}

void EndpointManager::AddConnection(string host, short port)
{
	JsonRpcClient::Ptr client = make_shared<JsonRpcClient>();
	RegisterClient(client);

	client->MakeSocket();
	client->Connect(host, port);
	client->Start();
}

void EndpointManager::RegisterServer(JsonRpcServer::Ptr server)
{
	m_Servers.push_front(server);
	server->OnNewClient += bind_weak(&EndpointManager::NewClientHandler, shared_from_this());
}

void EndpointManager::UnregisterServer(JsonRpcServer::Ptr server)
{
	m_Servers.remove(server);
	// TODO: unbind event
}

void EndpointManager::RegisterClient(JsonRpcClient::Ptr client)
{
	m_Clients.push_front(client);
	client->OnNewMessage += bind_weak(&EndpointManager::NewMessageHandler, shared_from_this());
	client->OnClosed += bind_weak(&EndpointManager::CloseClientHandler, shared_from_this());
	client->OnError += bind_weak(&EndpointManager::ErrorClientHandler, shared_from_this());
}

void EndpointManager::UnregisterClient(JsonRpcClient::Ptr client)
{
	m_Clients.remove(client);
	// TODO: unbind event
}

int EndpointManager::NewClientHandler(NewClientEventArgs::Ptr ncea)
{
	JsonRpcClient::Ptr client = static_pointer_cast<JsonRpcClient>(ncea->Client);
	RegisterClient(client);

	return 0;
}

int EndpointManager::CloseClientHandler(EventArgs::Ptr ea)
{
	JsonRpcClient::Ptr client = static_pointer_cast<JsonRpcClient>(ea->Source);
	UnregisterClient(client);

	if (client->GetPeerHost() != string()) {
		Timer::Ptr timer = make_shared<Timer>();
		timer->SetInterval(30);
		timer->SetUserArgs(ea);
		timer->OnTimerExpired += bind_weak(&EndpointManager::ReconnectClientHandler, shared_from_this());
		timer->Start();
		m_ReconnectTimers.push_front(timer);
	}

	return 0;
}

int EndpointManager::ErrorClientHandler(SocketErrorEventArgs::Ptr ea)
{
	cout << "Error occured for JSON-RPC socket: Code=" << ea->Code << "; Message=" << ea->Message << endl;

	return 0;
}

int EndpointManager::ReconnectClientHandler(TimerEventArgs::Ptr ea)
{
	JsonRpcClient::Ptr client = static_pointer_cast<JsonRpcClient>(ea->UserArgs->Source);
	Timer::Ptr timer = static_pointer_cast<Timer>(ea->Source);

	AddConnection(client->GetPeerHost(), client->GetPeerPort());

	timer->Stop();
	m_ReconnectTimers.remove(timer);

	return 0;
}

int EndpointManager::NewMessageHandler(NewMessageEventArgs::Ptr nmea)
{
// TODO: implement

	return 0;
}

void EndpointManager::RegisterEndpoint(Endpoint::Ptr endpoint)
{
	m_Endpoints.push_front(endpoint);
}

void EndpointManager::UnregisterEndpoint(Endpoint::Ptr endpoint)
{
	m_Endpoints.remove(endpoint);
}

void EndpointManager::SendMessage(Endpoint::Ptr source, Endpoint::Ptr destination, JsonRpcMessage::Ptr message)
{
	if (destination) {
		destination->SendMessage(source, message);
	} else {
		for (list<Endpoint::Ptr>::iterator i = m_Endpoints.begin(); i != m_Endpoints.end(); i++)
		{
			Endpoint::Ptr endpoint = *i;

			if (endpoint == source)
				continue;

			endpoint->SendMessage(source, message);
		}
	}
}
