#include "i2-base.h"

using namespace icinga;

TCPServer::TCPServer(void)
{
	m_ClientFactory = factory<TCPClient>;
}

void TCPServer::SetClientFactory(factory_function clientFactory)
{
	m_ClientFactory = clientFactory;
}

factory_function TCPServer::GetFactoryFunction(void)
{
	return m_ClientFactory;
}

void TCPServer::Start(void)
{
	TCPSocket::Start();

	OnReadable += bind_weak(&TCPServer::ReadableEventHandler, shared_from_this());
}

void TCPServer::Listen(void)
{
	int rc = listen(GetFD(), SOMAXCONN);

	if (rc < 0) {
		Close();
		return;
	}

	Start();
}

int TCPServer::ReadableEventHandler(const EventArgs& ea)
{
	int fd;
	sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	fd = accept(GetFD(), (sockaddr *)&addr, &addrlen);

	if (fd == INVALID_SOCKET) {
#ifdef _WIN32
		if (WSAGetLastError() == WSAEWOULDBLOCK)
#else /* _WIN32 */
		if (errno == EINPROGRESS)
#endif /* _WIN32 */
			return 0;

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

	NewClientEventArgs nea;
	nea.Source = shared_from_this();
	nea.Client = static_pointer_cast<TCPSocket>(m_ClientFactory());
	nea.Client->SetFD(fd);
	nea.Client->Start();
	OnNewClient(nea);

	return 0;
}

bool TCPServer::WantsToRead(void) const
{
	return true;
}
