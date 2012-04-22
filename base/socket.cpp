#include "i2-base.h"

using namespace icinga;

Socket::CollectionType Socket::Sockets;

Socket::Socket(void)
{
	m_FD = INVALID_SOCKET;
}

Socket::~Socket(void)
{
	Close(true);
}

void Socket::Start(void)
{
	assert(m_FD != INVALID_SOCKET);

	OnException += bind_weak(&Socket::ExceptionEventHandler, shared_from_this());

	Sockets.insert(static_pointer_cast<Socket>(shared_from_this()));
}

void Socket::Stop(void)
{
	Socket::Ptr self = static_pointer_cast<Socket>(shared_from_this());
	Socket::CollectionType::iterator i = Sockets.find(self);

	if (i != Sockets.end())
		Sockets.erase(i);
}

void Socket::SetFD(SOCKET fd)
{
	unsigned long lTrue = 1;

	if (fd != INVALID_SOCKET)
		ioctlsocket(fd, FIONBIO, &lTrue);

	m_FD = fd;
}

SOCKET Socket::GetFD(void) const
{
	return m_FD;
}

void Socket::Close(void)
{
	Close(false);
}

void Socket::Close(bool from_dtor)
{
	if (m_FD != INVALID_SOCKET) {
		closesocket(m_FD);
		m_FD = INVALID_SOCKET;

		/* nobody can possibly have a valid event subscription when the
		   destructor has been called */
		if (!from_dtor) {
			EventArgs ea;
			ea.Source = shared_from_this();
			OnClosed(ea);
		}
	}

	if (!from_dtor)
		Stop();
}

void Socket::HandleSocketError(void)
{
	int opt;
	socklen_t optlen = sizeof(opt);

	int rc = getsockopt(GetFD(), SOL_SOCKET, SO_ERROR, (char *)&opt, &optlen);

	if (rc >= 0 && opt != 0) {
		SocketErrorEventArgs sea;
		sea.Code = opt;
#ifdef _WIN32
		sea.Message = Win32Exception::FormatErrorCode(sea.Code);
#else /* _WIN32 */
		sea.Message = PosixException::FormatErrorCode(sea.Code);
#endif /* _WIN32 */
		OnError(sea);
	}

	Close();
	return;
}

int Socket::ExceptionEventHandler(const EventArgs& ea)
{
	HandleSocketError();

	return 0;
}

bool Socket::WantsToRead(void) const
{
	return false;
}

bool Socket::WantsToWrite(void) const
{
	return false;
}
