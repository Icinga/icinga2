#include "i2-icinga.h"

using namespace icinga;

string JsonRpcEndpoint::GetAddress(void) const
{
	if (!m_Client)
		return "<disconnected endpoint>";

	return m_Client->GetPeerAddress();
}

JsonRpcClient::Ptr JsonRpcEndpoint::GetClient(void)
{
	return m_Client;
}

void JsonRpcEndpoint::AddAllowedMethodSinkPrefix(string method)
{
	m_AllowedMethodSinkPrefixes.insert(method);
}

void JsonRpcEndpoint::RemoveAllowedMethodSinkPrefix(string method)
{
	m_AllowedMethodSinkPrefixes.erase(method);
}

bool JsonRpcEndpoint::IsAllowedMethodSink(string method) const
{
	set<string>::iterator i;
	for (i = m_AllowedMethodSinkPrefixes.begin(); i != m_AllowedMethodSinkPrefixes.end(); i++) {
		if (method.compare(0, method.length(), method) == 0)
			return true;
	}

	return false;
}

void JsonRpcEndpoint::AddAllowedMethodSourcePrefix(string method)
{
	m_AllowedMethodSourcePrefixes.insert(method);
}

void JsonRpcEndpoint::RemoveAllowedMethodSourcePrefix(string method)
{
	m_AllowedMethodSourcePrefixes.erase(method);
}

bool JsonRpcEndpoint::IsAllowedMethodSource(string method) const
{
	set<string>::iterator i;
	for (i = m_AllowedMethodSourcePrefixes.begin(); i != m_AllowedMethodSourcePrefixes.end(); i++) {
		if (method.compare(0, method.length(), method) == 0)
			return true;
	}

	return false;
}

void JsonRpcEndpoint::Connect(string host, unsigned short port, shared_ptr<SSL_CTX> sslContext)
{
	m_PeerHostname = host;
	m_PeerPort = port;

	JsonRpcClient::Ptr client = make_shared<JsonRpcClient>(RoleOutbound, sslContext);
	client->MakeSocket();
	client->Connect(host, port);
	client->Start();

	SetClient(client);
}

void JsonRpcEndpoint::SetClient(JsonRpcClient::Ptr client)
{
	m_Client = client;
	client->OnNewMessage += bind_weak(&JsonRpcEndpoint::NewMessageHandler, shared_from_this());
	client->OnClosed += bind_weak(&JsonRpcEndpoint::ClientClosedHandler, shared_from_this());
	client->OnError += bind_weak(&JsonRpcEndpoint::ClientErrorHandler, shared_from_this());
	client->OnVerifyCertificate += bind_weak(&JsonRpcEndpoint::VerifyCertificateHandler, shared_from_this());
}

bool JsonRpcEndpoint::IsLocal(void) const
{
	return false;
}

bool JsonRpcEndpoint::IsConnected(void) const
{
	return (bool)m_Client;
}

void JsonRpcEndpoint::ProcessRequest(Endpoint::Ptr sender, const JsonRpcRequest& message)
{
	if (IsConnected()) {
		string id;
		if (message.GetID(&id))
			// TODO: remove calls after a certain timeout (and notify callers?)
			m_PendingCalls[id] = sender;

		m_Client->SendMessage(message);
	} else {
		// TODO: persist the event
	}
}

void JsonRpcEndpoint::ProcessResponse(Endpoint::Ptr sender, const JsonRpcResponse& message)
{
	if (IsConnected())
		m_Client->SendMessage(message);
}

int JsonRpcEndpoint::NewMessageHandler(const NewMessageEventArgs& nmea)
{
	const Message& message = nmea.Message;
	Endpoint::Ptr sender = static_pointer_cast<Endpoint>(shared_from_this());

	string method;
	if (message.GetPropertyString("method", &method)) {
		if (!IsAllowedMethodSource(method))
			return 0;

		JsonRpcRequest request = message;

		string id;
		if (request.GetID(&id))
			GetEndpointManager()->SendAnycastRequest(sender, request, false);
		else
			GetEndpointManager()->SendMulticastRequest(sender, request, false);
	} else {
		JsonRpcResponse response = message;

		// TODO: deal with response messages
		throw NotImplementedException();
	}

	return 0;
}

int JsonRpcEndpoint::ClientClosedHandler(const EventArgs& ea)
{
	string address = GetAddress();
	Application::Log("Lost connection to endpoint: " + address);

	m_PendingCalls.clear();

	if (m_PeerHostname != string()) {
		Timer::Ptr timer = make_shared<Timer>();
		timer->SetInterval(30);
		timer->SetUserArgs(ea);
		timer->OnTimerExpired += bind_weak(&JsonRpcEndpoint::ClientReconnectHandler, shared_from_this());
		timer->Start();
		m_ReconnectTimer = timer;

		Application::Log("Spawned reconnect timer (30 seconds)");
	}

	// TODO: _only_ clear non-persistent method sources/sinks
	// unregister ourselves if no persistent sources/sinks are left (use a timer for that, once we have a TTL property for the methods)
	ClearMethodSinks();
	ClearMethodSources();

	if (CountMethodSinks() == 0 && !m_ReconnectTimer)
		GetEndpointManager()->UnregisterEndpoint(static_pointer_cast<Endpoint>(shared_from_this()));

	m_Client.reset();

	// TODO: persist events, etc., for now we just disable the endpoint

	return 0;
}

int JsonRpcEndpoint::ClientErrorHandler(const SocketErrorEventArgs& ea)
{
	cerr << "Error occured for JSON-RPC socket: Code=" << ea.Code << "; Message=" << ea.Message << endl;

	return 0;
}

int JsonRpcEndpoint::ClientReconnectHandler(const TimerEventArgs& ea)
{
	JsonRpcClient::Ptr client = static_pointer_cast<JsonRpcClient>(ea.UserArgs.Source);
	Timer::Ptr timer = static_pointer_cast<Timer>(ea.Source);

	GetEndpointManager()->AddConnection(m_PeerHostname, m_PeerPort);

	timer->Stop();
	m_ReconnectTimer.reset();

	return 0;
}

int JsonRpcEndpoint::VerifyCertificateHandler(const VerifyCertificateEventArgs& ea)
{
	if (ea.Certificate && ea.ValidCertificate) {
		string identity = Utility::GetCertificateCN(ea.Certificate);

		if (GetIdentity().empty() && !identity.empty())
			SetIdentity(identity);
	}

	return 0;
}
