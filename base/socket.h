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

#ifndef SOCKET_H
#define SOCKET_H

namespace icinga {

/**
 * Base class for sockets.
 *
 * @ingroup base
 */
class I2_BASE_API Socket : public Object
{
public:
	typedef shared_ptr<Socket> Ptr;
	typedef weak_ptr<Socket> WeakPtr;

	~Socket(void);

	boost::signal<void (const Socket::Ptr&)> OnClosed;

	virtual void Start(void);

	void Close(void);

	string GetClientAddress(void);
	string GetPeerAddress(void);

	mutex& GetMutex(void) const;

	bool IsConnected(void) const;

	void CheckException(void);

protected:
	Socket(void);

	void SetFD(SOCKET fd);
	SOCKET GetFD(void) const;

	void SetConnected(bool connected);

	int GetError(void) const;
	static int GetLastSocketError(void);

	virtual bool WantsToRead(void) const;
	virtual bool WantsToWrite(void) const;

	virtual void HandleReadable(void);
	virtual void HandleWritable(void);
	virtual void HandleException(void);

	virtual void CloseInternal(bool from_dtor);

private:
	SOCKET m_FD; /**< The socket descriptor. */
	bool m_Connected;

	thread m_ReadThread;
	thread m_WriteThread;

	condition_variable m_WriteCV;

	mutable mutex m_Mutex;
	boost::exception_ptr m_Exception;

	void ReadThreadProc(void);
	void WriteThreadProc(void);

	void ExceptionEventHandler(void);

	static string GetAddressFromSockaddr(sockaddr *address, socklen_t len);
};

/**
 * A socket exception.
 */
class SocketException : public Exception
{
public:
	SocketException(const string& message, int errorCode);
};

}

#endif /* SOCKET_H */
