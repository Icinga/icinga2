#include "i2-icinga.h"

using namespace icinga;

EndpointManager::EndpointManager(shared_ptr<SSL_CTX> sslContext)
{
	m_SSLContext = sslContext;
}

void EndpointManager::AddListener(unsigned short port)
{
	stringstream s;
	s << "Adding new listener: port " << port;
	Application::Log(s.str());

	JsonRpcServer::Ptr server = make_shared<JsonRpcServer>(m_SSLContext);
	RegisterServer(server);

	server->Bind(port, AF_INET6);
	server->Listen();
	server->Start();
}

void EndpointManager::AddConnection(string host, unsigned short port)
{
	stringstream s;
	s << "Adding new endpoint: [" << host << "]:" << port;
	Application::Log(s.str());

	JsonRpcEndpoint::Ptr endpoint = make_shared<JsonRpcEndpoint>();
	endpoint->Connect(host, port, m_SSLContext);
	RegisterEndpoint(endpoint);
}

void EndpointManager::RegisterServer(JsonRpcServer::Ptr server)
{
	m_Servers.push_front(server);
	server->OnNewClient += bind_weak(&EndpointManager::NewClientHandler, shared_from_this());
}

int EndpointManager::NewClientHandler(const NewClientEventArgs& ncea)
{
	string address = ncea.Client->GetPeerAddress();
	Application::Log("Accepted new client from " + address);

	JsonRpcEndpoint::Ptr endpoint = make_shared<JsonRpcEndpoint>();
	endpoint->SetClient(static_pointer_cast<JsonRpcClient>(ncea.Client));
	RegisterEndpoint(endpoint);

	return 0;
}

void EndpointManager::UnregisterServer(JsonRpcServer::Ptr server)
{
	m_Servers.remove(server);
	// TODO: unbind event
}

void EndpointManager::RegisterEndpoint(Endpoint::Ptr endpoint)
{
	endpoint->SetEndpointManager(static_pointer_cast<EndpointManager>(shared_from_this()));
	m_Endpoints.push_front(endpoint);

	endpoint->OnNewMethodSink += bind_weak(&EndpointManager::NewMethodSinkHandler, shared_from_this());
	endpoint->ForeachMethodSink(bind(&EndpointManager::NewMethodSinkHandler, this, _1));

	endpoint->OnNewMethodSource += bind_weak(&EndpointManager::NewMethodSourceHandler, shared_from_this());
	endpoint->ForeachMethodSource(bind(&EndpointManager::NewMethodSourceHandler, this, _1));

	NewEndpointEventArgs neea;
	neea.Source = shared_from_this();
	neea.Endpoint = endpoint;
	OnNewEndpoint(neea);
}

void EndpointManager::UnregisterEndpoint(Endpoint::Ptr endpoint)
{
	m_Endpoints.remove(endpoint);
}

void EndpointManager::SendUnicastRequest(Endpoint::Ptr sender, Endpoint::Ptr recipient, const JsonRpcRequest& request, bool fromLocal)
{
	if (sender == recipient)
		return;

	/* don't forward messages between non-local endpoints */
	if (!fromLocal && !recipient->IsLocal())
		return;

	string method;
	if (!request.GetMethod(&method))
		throw InvalidArgumentException("Missing 'method' parameter.");

	if (recipient->IsMethodSink(method) && recipient->IsAllowedMethodSink(method))
		recipient->ProcessRequest(sender, request);
}

void EndpointManager::SendAnycastRequest(Endpoint::Ptr sender, const JsonRpcRequest& request, bool fromLocal)
{
	throw NotImplementedException();
}

void EndpointManager::SendMulticastRequest(Endpoint::Ptr sender, const JsonRpcRequest& request, bool fromLocal)
{
#ifdef _DEBUG
	string id;
	if (request.GetID(&id))
		throw InvalidArgumentException("Multicast requests must not have an ID.");
#endif /* _DEBUG */

	string method;
	if (!request.GetMethod(&method))
		throw InvalidArgumentException();

	for (list<Endpoint::Ptr>::iterator i = m_Endpoints.begin(); i != m_Endpoints.end(); i++)
	{
		SendUnicastRequest(sender, *i, request, fromLocal);
	}
}

int EndpointManager::NewMethodSinkHandler(const NewMethodEventArgs& ea)
{
	Endpoint::Ptr sender = static_pointer_cast<Endpoint>(ea.Source);

	if (!sender->IsLocal())
		return 0;

	JsonRpcRequest request;
	request.SetMethod("message::Subscribe");

	SubscriptionMessage subscriptionMessage;
	subscriptionMessage.SetMethod(ea.Method);
	request.SetParams(subscriptionMessage);

	SendMulticastRequest(sender, request);

	return 0;
}

int EndpointManager::NewMethodSourceHandler(const NewMethodEventArgs& ea)
{
	Endpoint::Ptr sender = static_pointer_cast<Endpoint>(ea.Source);

	if (!sender->IsLocal())
		return 0;

	JsonRpcRequest request;
	request.SetMethod("message::Provide");

	SubscriptionMessage subscriptionMessage;
	subscriptionMessage.SetMethod(ea.Method);
	request.SetParams(subscriptionMessage);

	SendMulticastRequest(sender, request);

	return 0;
}

void EndpointManager::ForeachEndpoint(function<int (const NewEndpointEventArgs&)> callback)
{
	NewEndpointEventArgs neea;
	neea.Source = shared_from_this();
	for (list<Endpoint::Ptr>::iterator i = m_Endpoints.begin(); i != m_Endpoints.end(); i++) {
		neea.Endpoint = *i;
		callback(neea);
	}
}
