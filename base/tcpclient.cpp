#include "i2-base.h"

using namespace icinga;

TCPClient::TCPClient(TCPClientRole role)
{
	m_Role = role;

	m_SendQueue = make_shared<FIFO>();
	m_RecvQueue = make_shared<FIFO>();
}

TCPClientRole TCPClient::GetRole(void) const
{
	return m_Role;
}

void TCPClient::Start(void)
{
	TCPSocket::Start();

	OnReadable += bind_weak(&TCPClient::ReadableEventHandler, shared_from_this());
	OnWritable += bind_weak(&TCPClient::WritableEventHandler, shared_from_this());
}

void TCPClient::Connect(const string& hostname, unsigned short port)
{
	m_Role = RoleOutbound;

	stringstream s;
	s << port;
	string strPort = s.str();

	addrinfo hints;
	addrinfo *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	int rc = getaddrinfo(hostname.c_str(), strPort.c_str(), &hints, &result);

	if (rc < 0) {
		HandleSocketError();

		return;
	}

	int fd = INVALID_SOCKET;

	for (addrinfo *info = result; info != NULL; info = info->ai_next) {
		fd = socket(info->ai_family, info->ai_socktype, info->ai_protocol);

		if (fd == INVALID_SOCKET)
			continue;

		SetFD(fd);

		rc = connect(fd, info->ai_addr, info->ai_addrlen);

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

FIFO::Ptr TCPClient::GetSendQueue(void)
{
	return m_SendQueue;
}

FIFO::Ptr TCPClient::GetRecvQueue(void)
{
	return m_RecvQueue;
}

int TCPClient::ReadableEventHandler(const EventArgs& ea)
{
	int rc;

	size_t bufferSize = FIFO::BlockSize / 2;
	char *buffer = (char *)m_RecvQueue->GetWriteBuffer(&bufferSize);
	rc = recv(GetFD(), buffer, bufferSize, 0);

#ifdef _WIN32
	if (rc < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
#else /* _WIN32 */
	if (rc < 0 && errno == EAGAIN)
#endif /* _WIN32 */
		return 0;

	if (rc <= 0) {
		HandleSocketError();
		return 0;
	}

	m_RecvQueue->Write(NULL, rc);

	EventArgs dea;
	dea.Source = shared_from_this();
	OnDataAvailable(dea);

	return 0;
}

int TCPClient::WritableEventHandler(const EventArgs& ea)
{
	int rc;

	rc = send(GetFD(), (const char *)m_SendQueue->GetReadBuffer(), m_SendQueue->GetSize(), 0);

	if (rc <= 0) {
		HandleSocketError();
		return 0;
	}

	m_SendQueue->Read(NULL, rc);

	return 0;
}

bool TCPClient::WantsToRead(void) const
{
	return true;
}

bool TCPClient::WantsToWrite(void) const
{
	return (m_SendQueue->GetSize() > 0);
}

TCPClient::Ptr icinga::TCPClientFactory(TCPClientRole role)
{
	return make_shared<TCPClient>(role);
}
