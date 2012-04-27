#ifndef ENDPOINT_H
#define ENDPOINT_H

namespace icinga
{

class EndpointManager;

struct I2_ICINGA_API NewMethodEventArgs : public EventArgs
{
	string Method;
};

class I2_ICINGA_API Endpoint : public Object
{
private:
	string m_Identity;
	set<string> m_MethodSinks;
	set<string> m_MethodSources;

	weak_ptr<EndpointManager> m_EndpointManager;

public:
	typedef shared_ptr<Endpoint> Ptr;
	typedef weak_ptr<Endpoint> WeakPtr;

	virtual string GetAddress(void) const = 0;

	string GetIdentity(void) const;
	void SetIdentity(string identity);
	bool HasIdentity(void) const;

	shared_ptr<EndpointManager> GetEndpointManager(void) const;
	void SetEndpointManager(weak_ptr<EndpointManager> manager);

	void RegisterMethodSink(string method);
	void UnregisterMethodSink(string method);
	bool IsMethodSink(string method) const;

	void RegisterMethodSource(string method);
	void UnregisterMethodSource(string method);
	bool IsMethodSource(string method) const;
	
	virtual void AddAllowedMethodSinkPrefix(string method) = 0;
	virtual void RemoveAllowedMethodSinkPrefix(string method) = 0;
	virtual bool IsAllowedMethodSink(string method) const = 0;
	virtual void AddAllowedMethodSourcePrefix(string method) = 0;
	virtual void RemoveAllowedMethodSourcePrefix(string method) = 0;
	virtual bool IsAllowedMethodSource(string method) const = 0;

	virtual bool IsLocal(void) const = 0;
	virtual bool IsConnected(void) const = 0;

	virtual void ProcessRequest(Endpoint::Ptr sender, const JsonRpcRequest& message) = 0;
	virtual void ProcessResponse(Endpoint::Ptr sender, const JsonRpcResponse& message) = 0;

	virtual void Stop(void) = 0;

	Event<NewMethodEventArgs> OnNewMethodSink;
	Event<NewMethodEventArgs> OnNewMethodSource;

	void ForeachMethodSink(function<int (const NewMethodEventArgs&)> callback);
	void ForeachMethodSource(function<int (const NewMethodEventArgs&)> callback);

	void ClearMethodSinks(void);
	void ClearMethodSources(void);

	int CountMethodSinks(void) const;
	int CountMethodSources(void) const;

	Event<EventArgs> OnIdentityChanged;
};

}

#endif /* ENDPOINT_H */
