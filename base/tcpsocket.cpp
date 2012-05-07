#include "i2-base.h"

using namespace icinga;

void TCPSocket::MakeSocket(int family)
{
	assert(GetFD() == INVALID_SOCKET);

	int fd = socket(family, SOCK_STREAM, 0);

	if (fd == INVALID_SOCKET) {
		HandleSocketError();

		return;
	}

	SetFD(fd);
}

void TCPSocket::Bind(string service, int family)
{
	Bind(string(), service, family);
}

void TCPSocket::Bind(string node, string service, int family)
{
	addrinfo hints;
	addrinfo *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = family;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	if (getaddrinfo(node.empty() ? NULL : node.c_str(), service.c_str(), &hints, &result) < 0) {
		HandleSocketError();

		return;
	}

	int fd = INVALID_SOCKET;

	for (addrinfo *info = result; info != NULL; info = info->ai_next) {
		fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		if (fd == INVALID_SOCKET)
			continue;

		SetFD(fd);

		const int optFalse = 0;
		setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, (char *)&optFalse, sizeof(optFalse));

#ifndef _WIN32
		const int optTrue = 1;
		setsockopt(GetFD(), SOL_SOCKET, SO_REUSEADDR, (char *)&optTrue, sizeof(optTrue));
#endif /* _WIN32 */

		int rc = ::bind(fd, info->ai_addr, info->ai_addrlen);

#ifdef _WIN32
	if (rc < 0 && WSAGetLastError() != WSAEWOULDBLOCK)
#else /* _WIN32 */
	if (rc < 0 && errno != EINPROGRESS)
#endif /* _WIN32 */
			continue;

		break;
	}

	if (fd == INVALID_SOCKET)
		HandleSocketError();

	freeaddrinfo(result);
}
