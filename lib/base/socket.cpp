/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "i2-base.h"

using namespace icinga;

/**
 * Constructor for the Socket class.
 */
Socket::Socket(void)
	: m_FD(INVALID_SOCKET), m_Connected(false), m_Listening(false),
	 m_SendQueue(boost::make_shared<FIFO>()), m_RecvQueue(boost::make_shared<FIFO>())
{
	m_SendQueue->Start();
	m_RecvQueue->Start();
}

/**
 * Destructor for the Socket class.
 */
Socket::~Socket(void)
{
	m_SendQueue->Close();
	m_RecvQueue->Close();

	CloseInternal();
}

/**
 * Starts I/O processing for this socket.
 */
void Socket::Start(void)
{
	ASSERT(!m_ReadThread.joinable() && !m_WriteThread.joinable());
	ASSERT(GetFD() != INVALID_SOCKET);

	// TODO: figure out why we're not using "this" here
	m_ReadThread = thread(boost::bind(&Socket::ReadThreadProc, static_cast<Socket::Ptr>(GetSelf())));
	m_ReadThread.detach();

	m_WriteThread = thread(boost::bind(&Socket::WriteThreadProc, static_cast<Socket::Ptr>(GetSelf())));
	m_WriteThread.detach();

	Stream::Start();
}

/**
 * Sets the file descriptor for this socket object.
 *
 * @param fd The file descriptor.
 */
void Socket::SetFD(SOCKET fd)
{
	ObjectLock olock(this);

	/* mark the socket as non-blocking and close-on-exec */
	if (fd != INVALID_SOCKET) {
		Utility::SetNonBlockingSocket(fd);
#ifndef _WIN32
		Utility::SetCloExec(fd);
#endif /* _WIN32 */
	}

	m_FD = fd;
}

/**
 * Retrieves the file descriptor for this socket object.
 *
 * @returns The file descriptor.
 */
SOCKET Socket::GetFD(void) const
{
	ObjectLock olock(this);

	return m_FD;
}

/**
 * Closes the socket.
 */
void Socket::CloseInternal(void)
{
	{
		ObjectLock olock(this);

		if (m_FD != INVALID_SOCKET) {
			closesocket(m_FD);
			m_FD = INVALID_SOCKET;
		}
	}

	Stream::Close();
}

/**
 * Shuts down the socket.
 */
void Socket::Close(void)
{
	SetWriteEOF(true);
}


/**
 * Retrieves the last error that occured for the socket.
 *
 * @returns An error code.
 */
int Socket::GetError(void) const
{
	int opt;
	socklen_t optlen = sizeof(opt);

	int rc = getsockopt(GetFD(), SOL_SOCKET, SO_ERROR, (char *)&opt, &optlen);

	if (rc >= 0)
		return opt;

	return 0;
}

/**
 * Processes errors that have occured for the socket.
 */
void Socket::HandleException(void)
{
	ObjectLock olock(this);

#ifndef _WIN32
	BOOST_THROW_EXCEPTION(socket_error()
	    << errinfo_api_function("select")
	    << errinfo_errno(GetError()));
#else /* _WIN32 */
	BOOST_THROW_EXCEPTION(socket_error()
	    << errinfo_api_function("select")
	    << errinfo_win32_error(GetError()));
#endif /* _WIN32 */
}

/**
 * Formats a sockaddr in a human-readable way.
 *
 * @returns A String describing the sockaddr.
 */
String Socket::GetAddressFromSockaddr(sockaddr *address, socklen_t len)
{
	char host[NI_MAXHOST];
	char service[NI_MAXSERV];

	if (getnameinfo(address, len, host, sizeof(host), service,
	    sizeof(service), NI_NUMERICHOST | NI_NUMERICSERV) < 0) {
#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << errinfo_api_function("getnameinfo")
		    << errinfo_errno(errno));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
		    << errinfo_api_function("getnameinfo")
		    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	stringstream s;
	s << "[" << host << "]:" << service;
	return s.str();
}

/**
 * Returns a String describing the local address of the socket.
 *
 * @returns A String describing the local address.
 */
String Socket::GetClientAddress(void)
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	sockaddr_storage sin;
	socklen_t len = sizeof(sin);

	if (getsockname(GetFD(), (sockaddr *)&sin, &len) < 0) {
#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << errinfo_api_function("getsockname")
		    << errinfo_errno(errno));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
		    << errinfo_api_function("getsockname")
		    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	return GetAddressFromSockaddr((sockaddr *)&sin, len);
}

/**
 * Returns a String describing the peer address of the socket.
 *
 * @returns A String describing the peer address.
 */
String Socket::GetPeerAddress(void)
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	sockaddr_storage sin;
	socklen_t len = sizeof(sin);

	if (getpeername(GetFD(), (sockaddr *)&sin, &len) < 0) {
#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << errinfo_api_function("getpeername")
		    << errinfo_errno(errno));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
		    << errinfo_api_function("getpeername")
		    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	return GetAddressFromSockaddr((sockaddr *)&sin, len);
}

/**
 * Read thread procedure for sockets. This function waits until the
 * socket is readable and processes inbound data.
 */
void Socket::ReadThreadProc(void)
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	for (;;) {
		fd_set readfds, exceptfds;

		FD_ZERO(&readfds);
		FD_ZERO(&exceptfds);

		int fd = GetFD();

		if (fd == INVALID_SOCKET)
			return;

		if (WantsToRead())
			FD_SET(fd, &readfds);

		FD_SET(fd, &exceptfds);

		lock.unlock();

		timeval tv;
		tv.tv_sec = 5;
		tv.tv_usec = 0;
		int rc = select(fd + 1, &readfds, NULL, &exceptfds, &tv);

		lock.lock();

		if (GetFD() == INVALID_SOCKET)
			return;

		try {
			if (rc < 0) {
#ifndef _WIN32
				BOOST_THROW_EXCEPTION(socket_error()
				    << errinfo_api_function("select")
				    << errinfo_errno(errno));
#else /* _WIN32 */
				BOOST_THROW_EXCEPTION(socket_error()
				    << errinfo_api_function("select")
				    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
			}

			if (FD_ISSET(fd, &readfds))
				HandleReadable();

			if (FD_ISSET(fd, &exceptfds))
				HandleException();
		} catch (...) {
			SetException(boost::current_exception());

			CloseInternal();

			break;
		}

		if (WantsToWrite())
			m_WriteCV.notify_all(); /* notify Write thread */
	}
}

/**
 * Write thread procedure for sockets. This function waits until the socket
 * is writable and processes outbound data.
 */
void Socket::WriteThreadProc(void)
{
	boost::mutex::scoped_lock lock(m_SocketMutex);

	for (;;) {
		fd_set writefds;

		FD_ZERO(&writefds);

		while (!WantsToWrite()) {
			m_WriteCV.timed_wait(lock, boost::posix_time::seconds(1));

			if (GetFD() == INVALID_SOCKET)
				return;
		}

		int fd = GetFD();

		if (fd == INVALID_SOCKET)
			return;

		FD_SET(fd, &writefds);

		lock.unlock();

		int rc = select(fd + 1, NULL, &writefds, NULL, NULL);

		lock.lock();

		if (GetFD() == INVALID_SOCKET)
			return;

		try {
			if (rc < 0) {
#ifndef _WIN32
				BOOST_THROW_EXCEPTION(socket_error()
				    << errinfo_api_function("select")
				    << errinfo_errno(errno));
#else /* _WIN32 */
				BOOST_THROW_EXCEPTION(socket_error()
				    << errinfo_api_function("select")
				    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
			}

			if (FD_ISSET(fd, &writefds))
				HandleWritable();
		} catch (...) {
			SetException(boost::current_exception());

			CloseInternal();

			break;
		}
	}
}

/**
 * Sets whether the socket is fully connected.
 *
 * @param connected Whether the socket is connected
 */
void Socket::SetConnected(bool connected)
{
	ObjectLock olock(this);

	m_Connected = connected;
}

/**
 * Checks whether the socket is fully connected.
 *
 * @returns true if the socket is connected, false otherwise
 */
bool Socket::IsConnected(void) const
{
	ObjectLock olock(this);

	return m_Connected;
}

/**
 * Returns how much data is available for reading.
 *
 * @returns The number of bytes available.
 */
size_t Socket::GetAvailableBytes(void) const
{
	ObjectLock olock(this);

	if (m_Listening)
		throw new logic_error("Socket does not support GetAvailableBytes().");

	return m_RecvQueue->GetAvailableBytes();
}

/**
 * Reads data from the socket.
 *
 * @param buffer The buffer where the data should be stored.
 * @param size The size of the buffer.
 * @returns The number of bytes read.
 */
size_t Socket::Read(void *buffer, size_t size)
{
	{
		ObjectLock olock(this);

		if (m_Listening)
			throw new logic_error("Socket does not support Read().");
	}

	if (m_RecvQueue->GetAvailableBytes() == 0)
		CheckException();

	return m_RecvQueue->Read(buffer, size);
}

/**
 * Peeks at data for the socket.
 *
 * @param buffer The buffer where the data should be stored.
 * @param size The size of the buffer.
 * @returns The number of bytes read.
 */
size_t Socket::Peek(void *buffer, size_t size)
{
	{
		ObjectLock olock(this);

		if (m_Listening)
			throw new logic_error("Socket does not support Peek().");
	}

	if (m_RecvQueue->GetAvailableBytes() == 0)
		CheckException();

	return m_RecvQueue->Peek(buffer, size);
}

/**
 * Writes data to the socket.
 *
 * @param buffer The buffer that should be sent.
 * @param size The size of the buffer.
 */
void Socket::Write(const void *buffer, size_t size)
{
	{
		ObjectLock olock(this);

		if (m_Listening)
			throw new logic_error("Socket does not support Write().");
	}

	m_SendQueue->Write(buffer, size);
}

/**
 * Starts listening for incoming client connections.
 */
void Socket::Listen(void)
{
	if (listen(GetFD(), SOMAXCONN) < 0) {
#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << errinfo_api_function("listen")
		    << errinfo_errno(errno));
#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
		    << errinfo_api_function("listen")
		    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
	}

	{
		ObjectLock olock(this);
		m_Listening = true;
	}
}

void Socket::HandleWritable(void)
{
	if (m_Listening)
		HandleWritableServer();
	else
		HandleWritableClient();
}

void Socket::HandleReadable(void)
{
	if (m_Listening)
		HandleReadableServer();
	else
		HandleReadableClient();
}

/**
 * Processes data that is available for this socket.
 */
void Socket::HandleWritableClient(void)
{
	int rc;
	char data[1024];
	size_t count;

	if (!IsConnected())
		SetConnected(true);

	for (;;) {
		count = m_SendQueue->Peek(data, sizeof(data));

		if (count == 0) {
			if (IsWriteEOF())
				CloseInternal();

			break;
		}

		rc = send(GetFD(), data, count, 0);

		if (rc <= 0) {
#ifndef _WIN32
			BOOST_THROW_EXCEPTION(socket_error()
			    << errinfo_api_function("send")
			    << errinfo_errno(errno));
#else /* _WIN32 */
			BOOST_THROW_EXCEPTION(socket_error()
			    << errinfo_api_function("send")
			    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
		}

		m_SendQueue->Read(NULL, rc);
	}
}

/**
 * Processes data that can be written for this socket.
 */
void Socket::HandleReadableClient(void)
{
	if (!IsConnected())
		SetConnected(true);

	bool new_data = false;

	for (;;) {
		char data[1024];
		int rc = recv(GetFD(), data, sizeof(data), 0);

#ifdef _WIN32
		if (rc < 0 && WSAGetLastError() == WSAEWOULDBLOCK)
#else /* _WIN32 */
		if (rc < 0 && errno == EAGAIN)
#endif /* _WIN32 */
			break;

		if (rc < 0) {
#ifndef _WIN32
			BOOST_THROW_EXCEPTION(socket_error()
			    << errinfo_api_function("recv")
			    << errinfo_errno(errno));
#else /* _WIN32 */
			BOOST_THROW_EXCEPTION(socket_error()
			    << errinfo_api_function("recv")
			    << errinfo_win32_error(WSAGetLastError()));
#endif /* _WIN32 */
		}

		new_data = true;

		if (rc == 0) {
			SetReadEOF(true);

			break;
		}

		m_RecvQueue->Write(data, rc);
	}

	if (new_data)
		OnDataAvailable(GetSelf());
}

void Socket::HandleWritableServer(void)
{
	throw logic_error("This should never happen.");
}

/**
 * Accepts a new client and creates a new client object for it
 * using the client factory function.
 */
void Socket::HandleReadableServer(void)
{
	int fd;
	sockaddr_storage addr;
	socklen_t addrlen = sizeof(addr);

	fd = accept(GetFD(), (sockaddr *)&addr, &addrlen);

	if (fd < 0) {
	#ifndef _WIN32
		BOOST_THROW_EXCEPTION(socket_error()
		    << errinfo_api_function("accept")
		    << errinfo_errno(errno));
	#else /* _WIN32 */
		BOOST_THROW_EXCEPTION(socket_error()
		    << errinfo_api_function("accept")
		    << errinfo_win32_error(WSAGetLastError()));
	#endif /* _WIN32 */
	}

	Socket::Ptr client = boost::make_shared<Socket>();
	client->SetFD(fd);
	OnNewClient(GetSelf(), client);
}

/**
 * Checks whether data should be written for this socket object.
 *
 * @returns true if the socket should be registered for writing, false otherwise.
 */
bool Socket::WantsToWrite(void) const
{
	if (m_Listening)
		return WantsToWriteServer();
	else
		return WantsToWriteClient();
}

/**
 * Checks whether data should be read for this socket object.
 *
 * @returns true if the socket should be registered for reading, false otherwise.
 */
bool Socket::WantsToRead(void) const
{
	if (m_Listening)
		return WantsToReadServer();
	else
		return WantsToReadClient();
}

/**
 * Checks whether data should be read for this socket.
 *
 * @returns true
 */
bool Socket::WantsToReadClient(void) const
{
	return !IsReadEOF();
}

/**
 * Checks whether data should be written for this socket.
 *
 * @returns true if data should be written, false otherwise.
 */
bool Socket::WantsToWriteClient(void) const
{
	if (m_SendQueue->GetAvailableBytes() > 0)
		return true;

	return (!IsConnected());
}

/**
 * Checks whether the TCP server wants to write.
 *
 * @returns false
 */
bool Socket::WantsToWriteServer(void) const
{
	return false;
}

/**
 * Checks whether the TCP server wants to read (i.e. accept new clients).
 *
 * @returns true
 */
bool Socket::WantsToReadServer(void) const
{
	return true;
}
