#include "i2-base.h"

using namespace icinga;

void TCPSocket::MakeSocket(void)
{
	assert(GetFD() == INVALID_SOCKET);

	int fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd == INVALID_SOCKET) {
		HandleSocketError();

		return;
	}

	SetFD(fd);
}

void TCPSocket::Bind(unsigned short port)
{
	Bind(NULL, port);
}

void TCPSocket::Bind(const char *hostname, unsigned short port)
{
#ifndef _WIN32
	const int optTrue = 1;
	setsockopt(GetFD(), SOL_SOCKET, SO_REUSEADDR, (char *)&optTrue, sizeof(optTrue));
#endif /* _WIN32 */

	sockaddr_in sin;
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = hostname ? inet_addr(hostname) : htonl(INADDR_ANY);
	sin.sin_port = htons(port);

	if (::bind(GetFD(), (sockaddr *)&sin, sizeof(sin)) < 0)
		HandleSocketError();
}

string TCPSocket::GetAddressFromSockaddr(sockaddr *address)
{
	static char buffer[256];

#ifdef _WIN32
	DWORD BufferLength = sizeof(buffer);

	socklen_t len;
	if (address->sa_family == AF_INET)
		len = sizeof(sockaddr_in);
	else if (address->sa_family == AF_INET6)
		len = sizeof(sockaddr_in6);
	else {
		assert(0);

		return "";
	}

	if (WSAAddressToString(address, len, NULL, buffer, &BufferLength) != 0)
		return string();
#else /* _WIN32 */
	void *IpAddress;

	if (address->sa_family == AF_INET)
		IpAddress = &(((sockaddr_in *)address)->sin_addr);
	else
		IpAddress = &(((sockaddr_in6 *)address)->sin6_addr);

	if (inet_ntop(address->sa_family, address, buffer, sizeof(buffer)) == NULL)
		return string();
#endif /* _WIN32 */

	return buffer;
}

unsigned short TCPSocket::GetPortFromSockaddr(sockaddr *address)
{
	if (address->sa_family == AF_INET)
		return htons(((sockaddr_in *)address)->sin_port);
	else if (address->sa_family == AF_INET6)
		return htons(((sockaddr_in6 *)address)->sin6_port);
	else {
		assert(0);

		return 0;
	}
}

void TCPSocket::GetClientSockaddr(sockaddr_storage *address)
{
	socklen_t len = sizeof(*address);
	
	if (getsockname(GetFD(), (sockaddr *)address, &len) < 0)
		HandleSocketError();
}

void TCPSocket::GetPeerSockaddr(sockaddr_storage *address)
{
	socklen_t len = sizeof(*address);
	
	if (getpeername(GetFD(), (sockaddr *)address, &len) < 0)
		HandleSocketError();
}

string TCPSocket::GetClientAddress(void)
{
	sockaddr_storage sin;

	GetClientSockaddr(&sin);

	return GetAddressFromSockaddr((sockaddr *)&sin);
}

string TCPSocket::GetPeerAddress(void)
{
	sockaddr_storage sin;

	GetPeerSockaddr(&sin);

	return GetAddressFromSockaddr((sockaddr *)&sin);
}
