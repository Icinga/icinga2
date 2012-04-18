#include "i2-icinga.h"

using namespace icinga;

JsonRpcClient::Ptr JsonRpcEndpoint::GetClient(void)
{
	return m_Client;
}

void JsonRpcEndpoint::Connect(string host, unsigned short port)
{
	JsonRpcClient::Ptr client = make_shared<JsonRpcClient>();
	client->MakeSocket();
	client->Connect(host, port);
	client->Start();

	SetClient(client);
}

int JsonRpcEndpoint::SyncSubscription(string type, const NewMethodEventArgs& nmea)
{
	JsonRpcRequest request;
	request.SetVersion("2.0");
	request.SetMethod(type);

	Message params;
	params.GetDictionary()->SetValueString("method", nmea.Method);
	request.SetParams(params);

	m_Client->SendMessage(request);

	return 0;
}

int JsonRpcEndpoint::SyncSubscriptions(Endpoint::Ptr endpoint)
{
	if (!endpoint->IsLocal())
		return 0;

	endpoint->ForeachMethodSink(bind(&JsonRpcEndpoint::SyncSubscription, this, "message::Subscribe", _1));
	endpoint->ForeachMethodSource(bind(&JsonRpcEndpoint::SyncSubscription, this,  "message::Provide", _1));

	return 0;
}

void JsonRpcEndpoint::SetClient(JsonRpcClient::Ptr client)
{
	m_Client = client;
	client->OnNewMessage += bind_weak(&JsonRpcEndpoint::NewMessageHandler, shared_from_this());
	client->OnClosed += bind_weak(&JsonRpcEndpoint::ClientClosedHandler, shared_from_this());
	client->OnError += bind_weak(&JsonRpcEndpoint::ClientErrorHandler, shared_from_this());

	NewMethodEventArgs nmea;
	nmea.Source = shared_from_this();

	nmea.Method = "message::Subscribe";
	SyncSubscription("message::Subscribe", nmea);
	SyncSubscription("message::Provide", nmea);

	nmea.Method = "message::Provide";
	SyncSubscription("message::Subscribe", nmea);
	SyncSubscription("message::Provide", nmea);

	GetEndpointManager()->ForeachEndpoint(bind(&JsonRpcEndpoint::SyncSubscriptions, this, _1));
}

bool JsonRpcEndpoint::IsLocal(void) const
{
	return false;
}

bool JsonRpcEndpoint::IsConnected(void) const
{
	return (m_Client.get() != NULL);
}

void JsonRpcEndpoint::ProcessRequest(Endpoint::Ptr sender, const JsonRpcRequest& message)
{
	if (IsConnected()) {
		string id;
		if (message.GetID(&id))
			// TODO: remove calls after a certain timeout (and notify callers?)
			m_PendingCalls[id] = sender;

		m_Client->SendMessage(message);
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
	if (message.GetDictionary()->GetValueString("method", &method)) {
		JsonRpcRequest request = message;
		Message params;
		string method;

		if (request.GetMethod(&method) && request.GetParams(&params) &&
		    (method == "message::Subscribe" || method == "message::Provide")) {
			string sub_method;
			if (params.GetDictionary()->GetValueString("method", &sub_method)) {
				if (method == "message::Subscribe")
					RegisterMethodSink(sub_method);
				else
					RegisterMethodSource(sub_method);
			}

			return 0;
		}

		string id;
		if (request.GetID(&id))
			GetEndpointManager()->SendAnycastRequest(sender, request, false);
		else
			GetEndpointManager()->SendMulticastRequest(sender, request, false);
	} else {
		// TODO: deal with response messages
		throw NotImplementedException();
	}

	return 0;
}

int JsonRpcEndpoint::ClientClosedHandler(const EventArgs& ea)
{
	m_PendingCalls.clear();

	// TODO: clear method sources/sinks

	if (m_Client->GetPeerHost() != string()) {
		Timer::Ptr timer = make_shared<Timer>();
		timer->SetInterval(30);
		timer->SetUserArgs(ea);
		timer->OnTimerExpired += bind_weak(&JsonRpcEndpoint::ClientReconnectHandler, shared_from_this());
		timer->Start();
		m_ReconnectTimer = timer;
	}

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

	m_Client = client;

	timer->Stop();
	m_ReconnectTimer.reset();

	return 0;
}
