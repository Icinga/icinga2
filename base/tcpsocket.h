#ifndef TCPSOCKET_H
#define TCPSOCKET_H

namespace icinga
{

class TCPSocket : public Socket
{
public:
	typedef shared_ptr<TCPSocket> Ptr;
	typedef weak_ptr<TCPSocket> WeakPtr;

	void MakeSocket(void);

	void Bind(unsigned short port);
	void Bind(const char *hostname, unsigned short port);
};

}

#endif /* TCPSOCKET_H */
