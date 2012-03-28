#include <functional>
#include <algorithm>
#include "i2-base.h"

using namespace icinga;
using std::bind2nd;
using std::equal_to;

list<Socket::WeakRefType> Socket::Sockets;

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
	Sockets.push_front(static_pointer_cast<Socket>(shared_from_this()));
}

void Socket::Stop(void)
{
	Sockets.remove_if(weak_ptr_eq_raw<Socket>(this));
}

void Socket::SetFD(SOCKET fd)
{
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

		/* nobody can possibly have a valid event subscription when the destructor has been called */
		if (!from_dtor) {
			EventArgs::RefType ea = new_object<EventArgs>();
			ea->Source = shared_from_this();
			OnClosed(ea);
		}
	}

	if (!from_dtor)
		Stop();
}

void Socket::CloseAllSockets(void)
{
	for (list<Socket::WeakRefType>::iterator i = Sockets.begin(); i != Sockets.end(); ) {
		Socket::RefType socket = i->lock();

		i++;

		if (socket == NULL)
			continue;

		socket->Close();
	}
}

bool Socket::WantsToRead(void) const
{
	return false;
}

bool Socket::WantsToWrite(void) const
{
	return false;
}
