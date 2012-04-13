#include "i2-icinga.h"

using namespace icinga;

VirtualEndpoint::VirtualEndpoint()
{
	SetConnected(true);
}

void VirtualEndpoint::RegisterMethodHandler(string method, function<int (NewMessageEventArgs::Ptr)> callback)
{
	m_MethodHandlers[method] += callback;
}

void VirtualEndpoint::UnregisterMethodHandler(string method, function<int (NewMessageEventArgs::Ptr)> callback)
{
	// TODO: implement
	//m_Methods[method] -= callback;
}

void VirtualEndpoint::RegisterMethodSource(string method)
{
	m_MethodSources.push_front(method);
}

void VirtualEndpoint::UnregisterMethodSource(string method)
{
	m_MethodSources.remove(method);
}

void VirtualEndpoint::SendMessage(Endpoint::Ptr source, JsonRpcMessage::Ptr message)
{
	map<string, event<NewMessageEventArgs::Ptr> >::iterator i;
	i = m_MethodHandlers.find(message->GetMethod());

	if (i == m_MethodHandlers.end()) {
		JsonRpcMessage::Ptr response = make_shared<JsonRpcMessage>();
		response->SetVersion("2.0");
		response->SetError("Unknown method.");
		response->SetID(message->GetID());
		source->SendMessage(static_pointer_cast<Endpoint>(shared_from_this()), response);
	}

	NewMessageEventArgs::Ptr nmea = make_shared<NewMessageEventArgs>();
	nmea->Source = shared_from_this();
	nmea->Message = message;
	i->second(nmea);
}
