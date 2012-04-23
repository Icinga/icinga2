#ifndef SOCKET_H
#define SOCKET_H

namespace icinga {

struct I2_BASE_API SocketErrorEventArgs : public EventArgs
{
	typedef shared_ptr<SocketErrorEventArgs> Ptr;
	typedef weak_ptr<SocketErrorEventArgs> WeakPtr;

	int Code;
	string Message;
};

class I2_BASE_API Socket : public Object
{
private:
	SOCKET m_FD;

	int ExceptionEventHandler(const EventArgs& ea);

protected:
	Socket(void);

	void HandleSocketError(void);
	void Close(bool from_dtor);

public:
	typedef shared_ptr<Socket> Ptr;
	typedef weak_ptr<Socket> WeakPtr;

	typedef list<Socket::WeakPtr> CollectionType;

	static Socket::CollectionType Sockets;

	~Socket(void);

	void SetFD(SOCKET fd);
	SOCKET GetFD(void) const;

	Event<EventArgs> OnReadable;
	Event<EventArgs> OnWritable;
	Event<EventArgs> OnException;

	Event<SocketErrorEventArgs> OnError;
	Event<EventArgs> OnClosed;

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	virtual void Start(void);
	virtual void Stop(void);

	void Close(void);
};

}

#endif /* SOCKET_H */
