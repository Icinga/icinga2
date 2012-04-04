#include "i2-base.h"

using namespace icinga;

void TCPSocket::MakeSocket(void)
{
	assert(GetFD() == INVALID_SOCKET);

	int fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd == INVALID_SOCKET) {
		SocketErrorEventArgs::Ptr ea = make_shared<SocketErrorEventArgs>();
#ifdef _WIN32
		ea->Code = WSAGetLastError();
#else /* _WIN32 */
		ea->Code = errno;
#endif /* _WIN32 */
		ea->Message = FormatErrorCode(ea->Code);
		OnError(ea);
	}

	SetFD(fd);
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

	int rc = ::bind(GetFD(), (sockaddr *)&sin, sizeof(sin));

	if (rc < 0) {
		SocketErrorEventArgs::Ptr ea = make_shared<SocketErrorEventArgs>();
#ifdef _WIN32
		ea->Code = WSAGetLastError();
#else /* _WIN32 */
		ea->Code = errno;
#endif /* _WIN32 */
		ea->Message = FormatErrorCode(ea->Code);

		OnError(ea);
		
		Close();
	}
}
