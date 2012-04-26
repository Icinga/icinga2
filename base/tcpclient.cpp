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
	hostent *hent;
	sockaddr_in sin;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);

	hent = gethostbyname(hostname.c_str());

	if (hent != NULL)
		sin.sin_addr.s_addr = ((in_addr *)hent->h_addr_list[0])->s_addr;
	else
		sin.sin_addr.s_addr = inet_addr(hostname.c_str());

	int rc = connect(GetFD(), (sockaddr *)&sin, sizeof(sin));

#ifdef _WIN32
	if (rc < 0 && WSAGetLastError() != WSAEWOULDBLOCK) {
#else /* _WIN32 */
	if (rc < 0 && errno != EINPROGRESS) {
#endif /* _WIN32 */
		HandleSocketError();
	}

	m_Role = RoleOutbound;
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
