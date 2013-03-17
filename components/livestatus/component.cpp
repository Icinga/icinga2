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

#include "livestatus/component.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/tcpsocket.h"
#include "base/application.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;
using namespace livestatus;

REGISTER_TYPE(LivestatusComponent);

LivestatusComponent::LivestatusComponent(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{
	RegisterAttribute("socket_path", Attribute_Config, &m_SocketPath);
}

/**
 * Starts the component.
 */
void LivestatusComponent::Start(void)
{
//#ifndef _WIN32
//	UnixSocket::Ptr socket = boost::make_shared<UnixSocket>();
//	socket->Bind(GetSocketPath());
//#else /* _WIN32 */
	TcpSocket::Ptr socket = boost::make_shared<TcpSocket>();
	socket->Bind("6558", AF_INET);
//#endif /* _WIN32 */

	socket->OnNewClient.connect(boost::bind(&LivestatusComponent::NewClientHandler, this, _2));
	socket->Listen();
	socket->Start();
	m_Listener = socket;
}

String LivestatusComponent::GetSocketPath(void) const
{
	Value socketPath = m_SocketPath;
	if (socketPath.IsEmpty())
		return Application::GetLocalStateDir() + "/run/icinga2/livestatus";
	else
		return socketPath;
}

void LivestatusComponent::NewClientHandler(const Socket::Ptr& client)
{
	Log(LogInformation, "livestatus", "Client connected");

	LivestatusConnection::Ptr lconnection = boost::make_shared<LivestatusConnection>(client);
	lconnection->OnClosed.connect(boost::bind(&LivestatusComponent::ClientClosedHandler, this, _1));

	m_Connections.insert(lconnection);
	client->Start();
}

void LivestatusComponent::ClientClosedHandler(const Connection::Ptr& connection)
{
	LivestatusConnection::Ptr lconnection = static_pointer_cast<LivestatusConnection>(connection);

	Log(LogInformation, "livestatus", "Client disconnected");
	m_Connections.erase(lconnection);
}
