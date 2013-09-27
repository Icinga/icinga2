/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "livestatus/listener.h"
#include "base/objectlock.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/tcpsocket.h"
#include "base/unixsocket.h"
#include "base/networkstream.h"
#include "base/application.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;
using namespace livestatus;

REGISTER_TYPE(LivestatusListener);

static int l_ClientsConnected = 0;
static int l_Connections = 0;
static boost::mutex l_ComponentMutex;

/**
 * Starts the component.
 */
void LivestatusListener::Start(void)
{
	DynamicObject::Start();

	if (GetSocketType() == "tcp") {
		TcpSocket::Ptr socket = boost::make_shared<TcpSocket>();
		socket->Bind(GetBindHost(), GetBindPort(), AF_INET);

		boost::thread thread(boost::bind(&LivestatusListener::ServerThreadProc, this, socket));
		thread.detach();
	}
	else if (GetSocketType() == "unix") {
#ifndef _WIN32
		UnixSocket::Ptr socket = boost::make_shared<UnixSocket>();
		socket->Bind(GetSocketPath());

		/* group must be able to write */
		if (chmod(GetSocketPath().CStr(), 0660) < 0) {
			Log(LogCritical, "livestatus", "Cannot chmod unix socket '" + GetSocketPath() + "' to 0660: " + strerror(errno));
			return;
		}

		boost::thread thread(boost::bind(&LivestatusListener::ServerThreadProc, this, socket));
		thread.detach();
#else
		/* no unix sockets on windows */
		Log(LogCritical, "livestatus", "Unix sockets are not supported on Windows.");
		return;
#endif
	}
}

String LivestatusListener::GetSocketType(void) const
{
	Value socketType = m_SocketType;
	if (socketType.IsEmpty())
		return "unix";
	else
		return socketType;
}

String LivestatusListener::GetSocketPath(void) const
{
	Value socketPath = m_SocketPath;
	if (socketPath.IsEmpty())
		return Application::GetLocalStateDir() + "/run/icinga2/rw/livestatus";
	else
		return socketPath;
}

String LivestatusListener::GetBindHost(void) const
{
	if (m_BindHost.IsEmpty())
		return "127.0.0.1";
	else
		return m_BindHost;
}

String LivestatusListener::GetBindPort(void) const
{
	if (m_BindPort.IsEmpty())
		return "6558";
	else
		return m_BindPort;
}

int LivestatusListener::GetClientsConnected(void)
{
	boost::mutex::scoped_lock lock(l_ComponentMutex);

	return l_ClientsConnected;
}

int LivestatusListener::GetConnections(void)
{
	boost::mutex::scoped_lock lock(l_ComponentMutex);

	return l_Connections;
}

void LivestatusListener::ServerThreadProc(const Socket::Ptr& server)
{
	server->Listen();

	for (;;) {
		Socket::Ptr client = server->Accept();

		Log(LogInformation, "livestatus", "Client connected");

		boost::thread thread(boost::bind(&LivestatusListener::ClientThreadProc, this, client));
		thread.detach();
	}
}

void LivestatusListener::ClientThreadProc(const Socket::Ptr& client)
{
	{
		boost::mutex::scoped_lock lock(l_ComponentMutex);
		l_ClientsConnected++;
		l_Connections++;
	}

	Stream::Ptr stream = boost::make_shared<NetworkStream>(client);

	for (;;) {
		String line;
		ReadLineContext context;

		std::vector<String> lines;

		while (stream->ReadLine(&line, context)) {
			if (line.GetLength() > 0)
				lines.push_back(line);
			else
				break;
		}

		Query::Ptr query = boost::make_shared<Query>(lines);
		if (!query->Execute(stream))
			break;
	}

	{
		boost::mutex::scoped_lock lock(l_ComponentMutex);
		l_ClientsConnected--;
	}
}

void LivestatusListener::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("socket_type", m_SocketType);
		bag->Set("socket_path", m_SocketPath);
		bag->Set("bind_host", m_BindHost);
		bag->Set("bind_port", m_BindPort);
	}
}

void LivestatusListener::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_SocketType = bag->Get("socket_type");
		m_SocketPath = bag->Get("socket_path");
		m_BindHost = bag->Get("bind_host");
		m_BindPort = bag->Get("bind_port");
	}
}
