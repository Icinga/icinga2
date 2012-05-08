#include "i2-base.h"

using namespace icinga;

/**
 * TCPServer
 *
 * Constructor for the TCPServer class.
 */
TCPServer::TCPServer(void)
{
	m_ClientFactory = bind(&TCPClientFactory, RoleInbound);
}

/**
 * SetClientFactory
 *
 * Sets the client factory.
 *
 * @param clientFactory The client factory function.
 */
void TCPServer::SetClientFactory(function<TCPClient::Ptr()> clientFactory)
{
	m_ClientFactory = clientFactory;
}

/**
 * GetFactoryFunction
 *
 * Retrieves the client factory.
 *
 * @returns The client factory function.
 */
function<TCPClient::Ptr()> TCPServer::GetFactoryFunction(void) const
{
	return m_ClientFactory;
}

/**
 * Start
 *
 * Registers the TCP server and starts processing events for it.
 */
void TCPServer::Start(void)
{
	TCPSocket::Start();

	OnReadable += bind_weak(&TCPServer::ReadableEventHandler, shared_from_this());
}

/**
 * Listen
 *
 * Starts listening for incoming client connections.
 */
void TCPServer::Listen(void)
{
	int rc = listen(GetFD(), SOMAXCONN);

	if (rc < 0) {
		HandleSocketError();
		return;
	}
}

/**
 * ReadableEventHandler
 *
 * Accepts a new client and creates a new client object for it
 * using the client factory function.
 *
 * @param ea Event arguments.
 * @returns 0
 */
int TCPServer::ReadableEventHandler(const EventArgs& ea)
{
	int fd;
	sockaddr_storage addr;
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

/**
 * WantsToRead
 *
 * Checks whether the TCP server wants to read (i.e. accept new clients).
 *
 * @returns true
 */
bool TCPServer::WantsToRead(void) const
{
	return true;
}
