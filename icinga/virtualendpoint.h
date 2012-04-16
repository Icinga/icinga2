#ifndef VIRTUALENDPOINT_H
#define VIRTUALENDPOINT_H

namespace icinga
{

class I2_ICINGA_API VirtualEndpoint : public Endpoint
{
private:
	map< string, Event<NewMessageEventArgs::Ptr> > m_MethodHandlers;
	list<string> m_MethodSources;

public:
	typedef shared_ptr<VirtualEndpoint> Ptr;
	typedef weak_ptr<VirtualEndpoint> WeakPtr;

	VirtualEndpoint();

	virtual void RegisterMethodHandler(string method, function<int (NewMessageEventArgs::Ptr)> callback);
	virtual void UnregisterMethodHandler(string method, function<int (NewMessageEventArgs::Ptr)> callback);

	virtual void RegisterMethodSource(string method);
	virtual void UnregisterMethodSource(string method);

	virtual void SendMessage(Endpoint::Ptr source, JsonRpcMessage::Ptr message);
};

}

#endif /* VIRTUALENDPOINT_H */
