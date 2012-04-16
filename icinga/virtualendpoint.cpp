#include "i2-icinga.h"

using namespace icinga;

void VirtualEndpoint::RegisterMethodHandler(string method, function<int (NewRequestEventArgs::Ptr)> callback)
{
	m_MethodHandlers[method] += callback;

	RegisterMethodSink(method);
}

void VirtualEndpoint::UnregisterMethodHandler(string method, function<int (NewRequestEventArgs::Ptr)> callback)
{
	// TODO: implement
	//m_MethodHandlers[method] -= callback;
	//UnregisterMethodSink(method);

	throw NotImplementedException();
}

void VirtualEndpoint::SendRequest(Endpoint::Ptr sender, JsonRpcRequest::Ptr request)
{
	string method;
	if (!request->GetMethod(&method))
		return;

	map<string, Event<NewRequestEventArgs::Ptr> >::iterator i = m_MethodHandlers.find(method);

	if (i == m_MethodHandlers.end())
		throw InvalidArgumentException();

	NewRequestEventArgs::Ptr nrea = make_shared<NewRequestEventArgs>();
	nrea->Source = shared_from_this();
	nrea->Sender = sender;
	nrea->Request = request;
	i->second(nrea);
}

void VirtualEndpoint::SendResponse(Endpoint::Ptr sender, JsonRpcResponse::Ptr response)
{
	// TODO: figure out which request this response belongs to and notify the caller
	throw NotImplementedException();
}
