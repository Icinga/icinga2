#ifndef I2_SOCKET_H
#define I2_SOCKET_H

namespace icinga {

class Socket : public Object
{
protected:
	SOCKET m_FD;

	Socket(void);

	void Close(bool from_dtor);

public:
	typedef shared_ptr<Socket> RefType;
	typedef weak_ptr<Socket> WeakRefType;

	static list<Socket::WeakRefType> Sockets;

	~Socket(void);

	void SetFD(SOCKET fd);
	SOCKET GetFD(void) const;

	static void CloseAllSockets(void);

	event<EventArgs::RefType> OnReadable;
	event<EventArgs::RefType> OnWritable;
	event<EventArgs::RefType> OnException;

	event<EventArgs::RefType> OnClosed;

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	virtual void Start(void);
	virtual void Stop(void);

	void Close(void);
};

}

#endif /* I2_SOCKET_H */
