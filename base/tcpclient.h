#ifndef TCPCLIENT_H
#define TCPCLIENT_H

namespace icinga
{

class TCPClient : public TCPSocket
{
private:
	string m_PeerHost;
	int m_PeerPort;

	FIFO::Ptr m_SendQueue;
	FIFO::Ptr m_RecvQueue;

	int ReadableEventHandler(EventArgs::Ptr ea);
	int WritableEventHandler(EventArgs::Ptr ea);

public:
	typedef shared_ptr<TCPClient> Ptr;
	typedef weak_ptr<TCPClient> WeakPtr;

	TCPClient(void);

	virtual void Start(void);

	void Connect(const string& hostname, unsigned short port);

	FIFO::Ptr GetSendQueue(void);
	FIFO::Ptr GetRecvQueue(void);

	string GetPeerHost(void);
	int GetPeerPort(void);

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	event<EventArgs::Ptr> OnDataAvailable;
};

}

#endif /* TCPCLIENT_H */
