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
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.            *
 ******************************************************************************/

#ifndef TCPSOCKET_H
#define TCPSOCKET_H

namespace icinga
{

class I2_BASE_API TCPSocket : public Socket
{
private:
	void MakeSocket(int family);

public:
	typedef shared_ptr<TCPSocket> Ptr;
	typedef weak_ptr<TCPSocket> WeakPtr;

	void Bind(string service, int family);
	void Bind(string node, string service, int family);
};

}

#endif /* TCPSOCKET_H */
