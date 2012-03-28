#include "i2-base.h"

using namespace icinga;

void TCPSocket::MakeSocket(void)
{
	assert(m_FD == INVALID_SOCKET);

	m_FD = socket(AF_INET, SOCK_STREAM, 0);
}

void TCPSocket::Bind(unsigned short port)
{
	Bind(NULL, port);
}

void TCPSocket::Bind(const char *hostname, unsigned short port)
{
	sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = hostname ? inet_addr(hostname) : htonl(INADDR_ANY);
	sin.sin_port = htons(port);
	bind(GetFD(), (sockaddr *)&sin, sizeof(sin));
}
