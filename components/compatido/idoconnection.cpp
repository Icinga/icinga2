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

#include "i2-compatido.h"

using namespace icinga;

/**
 * Constructor for the IdoSocket class.
 *
 * @param stream The stream this connection should use.
 */
IdoConnection::IdoConnection(const Stream::Ptr& stream)
	: Connection(stream)
{ }

/**
 * Sends a message to the ido socket
 *
 * @param message The message.
 */
void IdoConnection::SendMessage(const String& message)
{
	GetStream()->Write(message.CStr(), message.GetLength());
}


/**
 * Processes inbound data.
 * Currently not used, as we do not receive data from ido sockets
 */
void IdoConnection::ProcessData(void)
{
	// Just ignore whatever data the other side is sending
	GetStream()->Read(NULL, GetStream()->GetAvailableBytes());

	return;
}
