#ifndef JSONRPCCLIENT_H
#define JSONRPCCLIENT_H

namespace icinga
{

struct NewMessageEventArgs : public EventArgs
{
	typedef shared_ptr<NewMessageEventArgs> RefType;
	typedef weak_ptr<NewMessageEventArgs> WeakRefType;

	JsonRpcMessage::RefType Message;
};

class JsonRpcClient : public TCPClient
{
private:
	int DataAvailableHandler(EventArgs::RefType ea);

public:
	typedef shared_ptr<JsonRpcClient> RefType;
	typedef weak_ptr<JsonRpcClient> WeakRefType;

	void SendMessage(JsonRpcMessage::RefType message);

	virtual void Start(void);

	event<NewMessageEventArgs::RefType> OnNewMessage;
};

}

#endif /* JSONRPCCLIENT_H */
