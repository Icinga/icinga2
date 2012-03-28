#ifndef I2_TCPSERVER_H
#define I2_TCPSERVER_H

namespace icinga
{

struct NewClientEventArgs : public EventArgs
{
	typedef shared_ptr<NewClientEventArgs> RefType;
	typedef weak_ptr<NewClientEventArgs> WeakRefType;

	TCPSocket::RefType Client;
};

class TCPServer : public TCPSocket
{
private:
	int ReadableEventHandler(EventArgs::RefType ea);

	factory_function m_ClientFactory;

public:
	typedef shared_ptr<TCPServer> RefType;
	typedef weak_ptr<TCPServer> WeakRefType;

	TCPServer(void);

	void SetClientFactory(factory_function function);
	factory_function GetFactoryFunction(void);

	virtual void Start();

	void Listen(void);

	event<NewClientEventArgs::RefType> OnNewClient;

	virtual bool WantsToRead(void) const;
};

}

#endif /* I2_TCPSERVER_H */
