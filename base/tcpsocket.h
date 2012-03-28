#ifndef I2_TCPSOCKET_H
#define I2_TCPSOCKET_H

namespace icinga
{

class TCPSocket : public Socket
{
public:
	typedef shared_ptr<TCPSocket> RefType;
	typedef weak_ptr<TCPSocket> WeakRefType;

	void MakeSocket(void);

	void Bind(unsigned short port);
	void Bind(const char *hostname, unsigned short port);
};

}

#endif /* I2_TCPSOCKET_H */