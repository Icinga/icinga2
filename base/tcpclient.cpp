#include "i2-base.h"

using namespace icinga;

TCPClient::TCPClient(void)
{
	m_SendQueue = new_object<FIFO>();
	m_RecvQueue = new_object<FIFO>();
}

void TCPClient::Start(void)
{
	TCPSocket::Start();

	OnReadable += bind_weak(&TCPClient::ReadableEventHandler, shared_from_this());
	OnWritable += bind_weak(&TCPClient::WritableEventHandler, shared_from_this());
}

FIFO::Ptr TCPClient::GetSendQueue(void)
{
	return m_SendQueue;
}

FIFO::Ptr TCPClient::GetRecvQueue(void)
{
	return m_RecvQueue;
}

int TCPClient::ReadableEventHandler(EventArgs::Ptr ea)
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
		Close();
		return 0;
	}

	m_RecvQueue->Write(NULL, rc);

	EventArgs::Ptr dea = new_object<EventArgs>();
	dea->Source = shared_from_this();
	OnDataAvailable(dea);

	return 0;
}

int TCPClient::WritableEventHandler(EventArgs::Ptr ea)
{
	int rc;

	rc = send(GetFD(), (const char *)m_SendQueue->GetReadBuffer(), m_SendQueue->GetSize(), 0);

	if (rc <= 0) {
		Close();
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
