#ifndef ENDPOINT_H
#define ENDPOINT_H

namespace icinga
{

class EndpointManager;

class I2_ICINGA_API Endpoint : public Object
{
private:
	set<string> m_MethodSinks;
	set<string> m_MethodSources;

public:
	typedef shared_ptr<Endpoint> Ptr;
	typedef weak_ptr<Endpoint> WeakPtr;

	void RegisterMethodSink(string method);
	void UnregisterMethodSink(string method);
	bool IsMethodSink(string method);

	void RegisterMethodSource(string method);
	void UnregisterMethodSource(string method);
	bool IsMethodSource(string method);

	virtual void SendRequest(Endpoint::Ptr sender, JsonRpcRequest::Ptr message) = 0;
	virtual void SendResponse(Endpoint::Ptr sender, JsonRpcResponse::Ptr message) = 0;
};

}

#endif /* ENDPOINT_H */
