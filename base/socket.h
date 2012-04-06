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

	int ExceptionEventHandler(EventArgs::Ptr ea);

protected:
	string FormatErrorCode(int errorCode);

protected:
	Socket(void);

	void Close(bool from_dtor);

public:
	typedef shared_ptr<Socket> Ptr;
	typedef weak_ptr<Socket> WeakPtr;

	static list<Socket::WeakPtr> Sockets;

	~Socket(void);

	void SetFD(SOCKET fd);
	SOCKET GetFD(void) const;

	static void CloseAllSockets(void);

	event<EventArgs::Ptr> OnReadable;
	event<EventArgs::Ptr> OnWritable;
	event<EventArgs::Ptr> OnException;

	event<SocketErrorEventArgs::Ptr> OnError;
	event<EventArgs::Ptr> OnClosed;

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	virtual void Start(void);
	virtual void Stop(void);

	void Close(void);
};

}

#endif /* SOCKET_H */
