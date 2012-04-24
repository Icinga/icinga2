#include "i2-base.h"

using namespace icinga;

TCPServer::TCPServer(void)
{
	m_ClientFactory = bind(&TCPClientFactory, RoleInbound);
}

void TCPServer::SetClientFactory(function<TCPClient::Ptr()> clientFactory)
{
	m_ClientFactory = clientFactory;
}

function<TCPClient::Ptr()> TCPServer::GetFactoryFunction(void) const
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
}

int TCPServer::ReadableEventHandler(const EventArgs& ea)
{
	int fd;
	sockaddr_in addr;
	socklen_t addrlen = sizeof(addr);

	fd = accept(GetFD(), (sockaddr *)&addr, &addrlen);

	if (fd < 0) {
		HandleSocketError();
		return 0;
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
