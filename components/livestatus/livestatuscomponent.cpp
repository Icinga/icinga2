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

#include "i2-livestatus.h"

using namespace icinga;

REGISTER_COMPONENT("livestatus", LivestatusComponent);

/**
 * Starts the component.
 */
void LivestatusComponent::Start(void)
{
	UnixSocket::Ptr socket = boost::make_shared<UnixSocket>();
	socket->OnNewClient.connect(boost::bind(&LivestatusComponent::NewClientHandler, this, _2));
	socket->Bind(GetSocketPath());
	socket->Listen();
	socket->Start();
	m_Listener = socket;
}

/**
 * Stops the component.
 */
void LivestatusComponent::Stop(void)
{
}

String LivestatusComponent::GetSocketPath(void) const
{
	DynamicObject::Ptr config = GetConfig();

	Value socketPath = config->Get("socket_path");
	if (socketPath.IsEmpty())
		return Application::GetLocalStateDir() + "/run/icinga2/livestatus";
	else
		return socketPath;
}

void LivestatusComponent::NewClientHandler(const Socket::Ptr& client)
{
	Logger::Write(LogInformation, "livestatus", "Client connected");

	LivestatusConnection::Ptr lconnection = boost::make_shared<LivestatusConnection>(client);
	lconnection->OnClosed.connect(boost::bind(&LivestatusComponent::ClientClosedHandler, this, _1));

	m_Connections.insert(lconnection);
	client->Start();
}

void LivestatusComponent::ClientClosedHandler(const Connection::Ptr& connection)
{
	LivestatusConnection::Ptr lconnection = static_pointer_cast<LivestatusConnection>(connection);

	Logger::Write(LogInformation, "livestatus", "Client disconnected");
	m_Connections.erase(lconnection);
}
