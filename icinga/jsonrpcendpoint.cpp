#include "i2-icinga.h"

using namespace icinga;

JsonRpcClient::Ptr JsonRpcEndpoint::GetClient(void)
{
	return m_Client;
}

void JsonRpcEndpoint::SetClient(JsonRpcClient::Ptr client)
{
	m_Client = client;
}

bool JsonRpcEndpoint::IsConnected(void) const
{
	return (m_Client.get() != NULL);
}

void JsonRpcEndpoint::SendRequest(Endpoint::Ptr sender, JsonRpcRequest::Ptr message)
{
	if (IsConnected())
		m_Client->SendMessage(message);
}

void JsonRpcEndpoint::SendResponse(Endpoint::Ptr sender, JsonRpcResponse::Ptr message)
{
	if (IsConnected())
		m_Client->SendMessage(message);
}
