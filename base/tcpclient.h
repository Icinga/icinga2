#ifndef TCPCLIENT_H
#define TCPCLIENT_H

namespace icinga
{

enum I2_BASE_API TCPClientRole
{
	RoleInbound,
	RoleOutbound
};

class I2_BASE_API TCPClient : public TCPSocket
{
private:
	TCPClientRole m_Role;

	FIFO::Ptr m_SendQueue;
	FIFO::Ptr m_RecvQueue;

	virtual int ReadableEventHandler(const EventArgs& ea);
	virtual int WritableEventHandler(const EventArgs& ea);

public:
	typedef shared_ptr<TCPClient> Ptr;
	typedef weak_ptr<TCPClient> WeakPtr;

	TCPClient(TCPClientRole role);

	TCPClientRole GetRole(void) const;

	virtual void Start(void);

	void Connect(const string& hostname, unsigned short port);

	FIFO::Ptr GetSendQueue(void);
	FIFO::Ptr GetRecvQueue(void);

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	Event<EventArgs> OnDataAvailable;
};

TCPClient::Ptr TCPClientFactory(TCPClientRole role);

}

#endif /* TCPCLIENT_H */
