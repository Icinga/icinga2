/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "livestatus/livestatuslistener.hpp"
#include "config/configcompilercontext.hpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include "base/dynamictype.hpp"
#include "base/logger_fwd.hpp"
#include "base/exception.hpp"
#include "base/tcpsocket.hpp"
#include "base/unixsocket.hpp"
#include "base/networkstream.hpp"
#include "base/application.hpp"
#include "base/scriptfunction.hpp"
#include "base/statsfunction.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_TYPE(LivestatusListener);
REGISTER_SCRIPTFUNCTION(ValidateSocketType, &LivestatusListener::ValidateSocketType);

static int l_ClientsConnected = 0;
static int l_Connections = 0;
static boost::mutex l_ComponentMutex;

REGISTER_STATSFUNCTION(LivestatusListenerStats, &LivestatusListener::StatsFunc);

Value LivestatusListener::StatsFunc(Dictionary::Ptr& status, Dictionary::Ptr& perfdata)
{
	Dictionary::Ptr nodes = make_shared<Dictionary>();

	BOOST_FOREACH(const LivestatusListener::Ptr& livestatuslistener, DynamicType::GetObjects<LivestatusListener>()) {
		Dictionary::Ptr stats = make_shared<Dictionary>();
		stats->Set("connections", l_Connections);

		nodes->Set(livestatuslistener->GetName(), stats);

		perfdata->Set("livestatuslistener_" + livestatuslistener->GetName() + "_connections", Convert::ToDouble(l_Connections));
	}

	status->Set("livestatuslistener", nodes);

	return 0;
}

/**
 * Starts the component.
 */
void LivestatusListener::Start(void)
{
	DynamicObject::Start();

	if (GetSocketType() == "tcp") {
		TcpSocket::Ptr socket = make_shared<TcpSocket>();
		socket->Bind(GetBindHost(), GetBindPort(), AF_UNSPEC);

		boost::thread thread(boost::bind(&LivestatusListener::ServerThreadProc, this, socket));
		thread.detach();
		Log(LogInformation, "LivestatusListener", "Created tcp socket listening on host '" + GetBindHost() + "' port '" + GetBindPort() + "'.");
	}
	else if (GetSocketType() == "unix") {
#ifndef _WIN32
		UnixSocket::Ptr socket = make_shared<UnixSocket>();
		socket->Bind(GetSocketPath());

		/* group must be able to write */
		mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

		if (chmod(GetSocketPath().CStr(), mode) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("chmod")
			    << boost::errinfo_errno(errno)
			    << boost::errinfo_file_name(GetSocketPath()));
		}

		boost::thread thread(boost::bind(&LivestatusListener::ServerThreadProc, this, socket));
		thread.detach();
		Log(LogInformation, "LivestatusListener", "Created unix socket in '" + GetSocketPath() + "'.");
#else
		/* no unix sockets on windows */
		Log(LogCritical, "LivestatusListener", "Unix sockets are not supported on Windows.");
		return;
#endif
	}
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

		Log(LogNotice, "LivestatusListener", "Client connected");

		Utility::QueueAsyncCallback(boost::bind(&LivestatusListener::ClientHandler, this, client));
	}
}

void LivestatusListener::ClientHandler(const Socket::Ptr& client)
{
	{
		boost::mutex::scoped_lock lock(l_ComponentMutex);
		l_ClientsConnected++;
		l_Connections++;
	}

	Stream::Ptr stream = make_shared<NetworkStream>(client);

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

		if (lines.empty())
			break;

		LivestatusQuery::Ptr query = make_shared<LivestatusQuery>(lines, GetCompatLogPath());
		if (!query->Execute(stream))
			break;
	}

	{
		boost::mutex::scoped_lock lock(l_ComponentMutex);
		l_ClientsConnected--;
	}
}


void LivestatusListener::ValidateSocketType(const String& location, const Dictionary::Ptr& attrs)
{
	Value socket_type = attrs->Get("socket_type");

	if (!socket_type.IsEmpty() && socket_type != "unix" && socket_type != "tcp") {
		ConfigCompilerContext::GetInstance()->AddMessage(true, "Validation failed for " +
		    location + ": Socket type '" + socket_type + "' is invalid.");
	}
}
