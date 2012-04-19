#ifndef JSONRPCENDPOINT_H
#define JSONRPCENDPOINT_H

namespace icinga
{

class I2_ICINGA_API JsonRpcEndpoint : public Endpoint
{
private:
	JsonRpcClient::Ptr m_Client;
	map<string, Endpoint::Ptr> m_PendingCalls;
	Timer::Ptr m_ReconnectTimer;

	bool IsConnected(void) const;
	
	int NewMessageHandler(const NewMessageEventArgs& nmea);
	int ClientClosedHandler(const EventArgs& ea);
	int ClientErrorHandler(const SocketErrorEventArgs& ea);
	int ClientReconnectHandler(const TimerEventArgs& ea);

public:
	typedef shared_ptr<JsonRpcEndpoint> Ptr;
	typedef weak_ptr<JsonRpcEndpoint> WeakPtr;

	void Connect(string host, unsigned short port);

	JsonRpcClient::Ptr GetClient(void);
	void SetClient(JsonRpcClient::Ptr client);

	virtual bool IsLocal(void) const;

	virtual void ProcessRequest(Endpoint::Ptr sender, const JsonRpcRequest& message);
	virtual void ProcessResponse(Endpoint::Ptr sender, const JsonRpcResponse& message);
};

}

#endif /* JSONRPCENDPOINT_H */
