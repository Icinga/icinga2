#ifndef TCPSOCKET_H
#define TCPSOCKET_H

namespace icinga
{

class I2_BASE_API TCPSocket : public Socket
{
private:
	static string GetAddressFromSockaddr(sockaddr *address);
	static unsigned short GetPortFromSockaddr(sockaddr *address);

public:
	typedef shared_ptr<TCPSocket> Ptr;
	typedef weak_ptr<TCPSocket> WeakPtr;

	void MakeSocket(void);

	void Bind(unsigned short port);
	void Bind(const char *hostname, unsigned short port);

	void GetClientSockaddr(sockaddr_storage *address);
	void GetPeerSockaddr(sockaddr_storage *address);

	string TCPSocket::GetClientAddress(void);
	string TCPSocket::GetPeerAddress(void);
};

}

#endif /* TCPSOCKET_H */
