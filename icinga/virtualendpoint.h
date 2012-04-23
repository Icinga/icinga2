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

	virtual void AddAllowedMethodSinkPrefix(string method);
	virtual void RemoveAllowedMethodSinkPrefix(string method);
	virtual bool IsAllowedMethodSink(string method) const;
	virtual void AddAllowedMethodSourcePrefix(string method);
	virtual void RemoveAllowedMethodSourcePrefix(string method);
	virtual bool IsAllowedMethodSource(string method) const;

	virtual string GetAddress(void) const;

	virtual bool IsLocal(void) const;

	virtual void ProcessRequest(Endpoint::Ptr sender, const JsonRpcRequest& message);
	virtual void ProcessResponse(Endpoint::Ptr sender, const JsonRpcResponse& message);
};

}

#endif /* VIRTUALENDPOINT_H */
