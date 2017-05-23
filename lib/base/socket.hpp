/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/i2-base.hpp"
#include "base/object.hpp"

namespace icinga
{

/**
 * Base class for connection-oriented sockets.
 *
 * @ingroup base
 */
class I2_BASE_API Socket : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Socket);

	Socket(void);
	Socket(SOCKET fd);
	~Socket(void);

	SOCKET GetFD(void) const;

	void Close(void);

	String GetClientAddress(void);
	String GetPeerAddress(void);

	size_t Read(void *buffer, size_t size);
	size_t Write(const void *buffer, size_t size);

	void Listen(void);
	Socket::Ptr Accept(void);

	bool Poll(bool read, bool write, struct timeval *timeout = NULL);

	void MakeNonBlocking(void);

	static void SocketPair(SOCKET s[2]);

protected:
	void SetFD(SOCKET fd);

	int GetError(void) const;

	mutable boost::mutex m_SocketMutex;

private:
	SOCKET m_FD; /**< The socket descriptor. */

	static String GetAddressFromSockaddr(sockaddr *address, socklen_t len);
};

class socket_error : virtual public std::exception, virtual public boost::exception { };

}

#endif /* SOCKET_H */
