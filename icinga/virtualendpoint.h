#ifndef VIRTUALENDPOINT_H
#define VIRTUALENDPOINT_H

namespace icinga
{

struct I2_JSONRPC_API NewRequestEventArgs : public EventArgs
{
	typedef shared_ptr<NewRequestEventArgs> Ptr;
	typedef weak_ptr<NewRequestEventArgs> WeakPtr;

	Endpoint::Ptr Sender;
	JsonRpcRequest::Ptr Request;
};

class I2_ICINGA_API VirtualEndpoint : public Endpoint
{
private:
	map< string, Event<NewRequestEventArgs::Ptr> > m_MethodHandlers;

public:
	typedef shared_ptr<VirtualEndpoint> Ptr;
	typedef weak_ptr<VirtualEndpoint> WeakPtr;

	void RegisterMethodHandler(string method, function<int (NewRequestEventArgs::Ptr)> callback);
	void UnregisterMethodHandler(string method, function<int (NewRequestEventArgs::Ptr)> callback);

	virtual void RegisterMethodSource(string method);
	virtual void UnregisterMethodSource(string method);

	virtual void SendRequest(Endpoint::Ptr sender, JsonRpcRequest::Ptr message);
	virtual void SendResponse(Endpoint::Ptr sender, JsonRpcResponse::Ptr message);
};

}

#endif /* VIRTUALENDPOINT_H */
