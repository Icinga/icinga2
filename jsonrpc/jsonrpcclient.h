#ifndef JSONRPCCLIENT_H
#define JSONRPCCLIENT_H

namespace icinga
{

struct I2_JSONRPC_API NewMessageEventArgs : public EventArgs
{
	typedef shared_ptr<NewMessageEventArgs> Ptr;
	typedef weak_ptr<NewMessageEventArgs> WeakPtr;

	icinga::Message Message;
};

class I2_JSONRPC_API JsonRpcClient : public TLSClient
{
private:
	int DataAvailableHandler(const EventArgs& ea);

public:
	typedef shared_ptr<JsonRpcClient> Ptr;
	typedef weak_ptr<JsonRpcClient> WeakPtr;

	JsonRpcClient(TCPClientRole role, shared_ptr<SSL_CTX> sslContext);

	void SendMessage(const Message& message);

	virtual void Start(void);

	Event<NewMessageEventArgs> OnNewMessage;
};

TCPClient::Ptr JsonRpcClientFactory(TCPClientRole role, shared_ptr<SSL_CTX> sslContext);

}

#endif /* JSONRPCCLIENT_H */
