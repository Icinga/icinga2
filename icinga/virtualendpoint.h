#ifndef VIRTUALENDPOINT_H
#define VIRTUALENDPOINT_H

namespace icinga
{

struct I2_ICINGA_API NewRequestEventArgs : public EventArgs
{
	typedef shared_ptr<NewRequestEventArgs> Ptr;
	typedef weak_ptr<NewRequestEventArgs> WeakPtr;

	Endpoint::Ptr Sender;
	JsonRpcRequest Request;
};

class I2_ICINGA_API VirtualEndpoint : public Endpoint
{
private:
	map< string, Event<NewRequestEventArgs> > m_MethodHandlers;

public:
	typedef shared_ptr<VirtualEndpoint> Ptr;
	typedef weak_ptr<VirtualEndpoint> WeakPtr;

	void RegisterMethodHandler(string method, function<int (const NewRequestEventArgs&)> callback);
	void UnregisterMethodHandler(string method, function<int (const NewRequestEventArgs&)> callback);

	virtual string GetAddress(void) const;

	virtual bool IsLocal(void) const;
	virtual bool IsConnected(void) const;

	virtual void ProcessRequest(Endpoint::Ptr sender, const JsonRpcRequest& message);
	virtual void ProcessResponse(Endpoint::Ptr sender, const JsonRpcResponse& message);

	virtual void Stop(void);
};

}

#endif /* VIRTUALENDPOINT_H */
