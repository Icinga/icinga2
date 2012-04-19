#include "i2-base.h"

using namespace icinga;

void TCPSocket::MakeSocket(void)
{
	assert(GetFD() == INVALID_SOCKET);

	int fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd == INVALID_SOCKET) {
		SocketErrorEventArgs sea;
#ifdef _WIN32
		sea.Code = WSAGetLastError();
#else /* _WIN32 */
		sea.Code = errno;
#endif /* _WIN32 */
		sea.Message = FormatErrorCode(sea.Code);
		OnError(sea);
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

	int rc = ::bind(GetFD(), (sockaddr *)&sin, sizeof(sin));

	if (rc < 0) {
		SocketErrorEventArgs sea;
#ifdef _WIN32
		sea.Code = WSAGetLastError();
#else /* _WIN32 */
		sea.Code = errno;
#endif /* _WIN32 */
		sea.Message = FormatErrorCode(sea.Code);

		OnError(sea);
		
		Close();
	}
}
