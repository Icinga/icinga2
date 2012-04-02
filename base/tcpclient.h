#ifndef TCPCLIENT_H
#define TCPCLIENT_H

namespace icinga
{

class TCPClient : public TCPSocket
{
private:
	FIFO::RefType m_SendQueue;
	FIFO::RefType m_RecvQueue;

	int ReadableEventHandler(EventArgs::RefType ea);
	int WritableEventHandler(EventArgs::RefType ea);

public:
	typedef shared_ptr<TCPClient> RefType;
	typedef weak_ptr<TCPClient> WeakRefType;

	TCPClient(void);

	virtual void Start(void);

	void Connect(const char *hostname, unsigned short port);

	FIFO::RefType GetSendQueue(void);
	FIFO::RefType GetRecvQueue(void);

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	event<EventArgs::RefType> OnDataAvailable;
};

}

#endif /* TCPCLIENT_H */
