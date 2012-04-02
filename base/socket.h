#ifndef SOCKET_H
#define SOCKET_H

namespace icinga {

class Socket : public Object
{
private:
	SOCKET m_FD;

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

	event<EventArgs::Ptr> OnClosed;

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	virtual void Start(void);
	virtual void Stop(void);

	void Close(void);
};

}

#endif /* SOCKET_H */
