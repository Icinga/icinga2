#ifndef JSONRPCCLIENT_H
#define JSONRPCCLIENT_H

namespace icinga
{

struct I2_JSONRPC_API NewMessageEventArgs : public EventArgs
{
	typedef shared_ptr<NewMessageEventArgs> Ptr;
	typedef weak_ptr<NewMessageEventArgs> WeakPtr;

	JsonRpcMessage::Ptr Message;
};

class I2_JSONRPC_API JsonRpcClient : public TCPClient
{
private:
	int DataAvailableHandler(EventArgs::Ptr ea);

public:
	typedef shared_ptr<JsonRpcClient> Ptr;
	typedef weak_ptr<JsonRpcClient> WeakPtr;

	void SendMessage(JsonRpcMessage::Ptr message);

	virtual void Start(void);

	Event<NewMessageEventArgs::Ptr> OnNewMessage;
};

}

#endif /* JSONRPCCLIENT_H */
