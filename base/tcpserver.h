#ifndef TCPSERVER_H
#define TCPSERVER_H

namespace icinga
{

struct NewClientEventArgs : public EventArgs
{
	typedef shared_ptr<NewClientEventArgs> Ptr;
	typedef weak_ptr<NewClientEventArgs> WeakPtr;

	TCPSocket::Ptr Client;
};

class TCPServer : public TCPSocket
{
private:
	int ReadableEventHandler(EventArgs::Ptr ea);

	factory_function m_ClientFactory;

public:
	typedef shared_ptr<TCPServer> Ptr;
	typedef weak_ptr<TCPServer> WeakPtr;

	TCPServer(void);

	void SetClientFactory(factory_function function);
	factory_function GetFactoryFunction(void);

	virtual void Start();

	void Listen(void);

	event<NewClientEventArgs::Ptr> OnNewClient;

	virtual bool WantsToRead(void) const;
};

}

#endif /* TCPSERVER_H */
