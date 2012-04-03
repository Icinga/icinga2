#include "i2-jsonrpc.h"

using namespace icinga;

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
	UnregisterClient(static_pointer_cast<JsonRpcClient>(ea->Source));

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
	m_Methods[method] -= callback;
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
