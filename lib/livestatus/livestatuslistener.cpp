/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "livestatus/livestatuslistener.tcpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/objectlock.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/tcpsocket.hpp"
#include "base/unixsocket.hpp"
#include "base/networkstream.hpp"
#include "base/application.hpp"
#include "base/function.hpp"
#include "base/statsfunction.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_TYPE(LivestatusListener);

static int l_ClientsConnected = 0;
static int l_Connections = 0;
static boost::mutex l_ComponentMutex;

REGISTER_STATSFUNCTION(LivestatusListener, &LivestatusListener::StatsFunc);

void LivestatusListener::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const LivestatusListener::Ptr& livestatuslistener : ConfigType::GetObjectsByType<LivestatusListener>()) {
		Dictionary::Ptr stats = new Dictionary();
		stats->Set("connections", l_Connections);

		nodes->Set(livestatuslistener->GetName(), stats);

		perfdata->Add(new PerfdataValue("livestatuslistener_" + livestatuslistener->GetName() + "_connections", l_Connections));
	}

	status->Set("livestatuslistener", nodes);
}

/**
 * Starts the component.
 */
void LivestatusListener::Start(bool runtimeCreated)
{
	ObjectImpl<LivestatusListener>::Start(runtimeCreated);

	Log(LogInformation, "LivestatusListener")
		<< "'" << GetName() << "' started.";

	if (GetSocketType() == "tcp") {
		TcpSocket::Ptr socket = new TcpSocket();

		try {
			socket->Bind(GetBindHost(), GetBindPort(), AF_UNSPEC);
		} catch (std::exception&) {
			Log(LogCritical, "LivestatusListener")
				<< "Cannot bind TCP socket on host '" << GetBindHost() << "' port '" << GetBindPort() << "'.";
			return;
		}

		m_Listener = socket;

		m_Thread = std::thread(std::bind(&LivestatusListener::ServerThreadProc, this));

		Log(LogInformation, "LivestatusListener")
			<< "Created TCP socket listening on host '" << GetBindHost() << "' port '" << GetBindPort() << "'.";
	}
	else if (GetSocketType() == "unix") {
#ifndef _WIN32
		UnixSocket::Ptr socket = new UnixSocket();

		try {
			socket->Bind(GetSocketPath());
		} catch (std::exception&) {
			Log(LogCritical, "LivestatusListener")
				<< "Cannot bind UNIX socket to '" << GetSocketPath() << "'.";
			return;
		}

		/* group must be able to write */
		mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

		if (chmod(GetSocketPath().CStr(), mode) < 0) {
			Log(LogCritical, "LivestatusListener")
				<< "chmod() on unix socket '" << GetSocketPath() << "' failed with error code " << errno << ", \"" << Utility::FormatErrorNumber(errno) << "\"";
			return;
		}

		m_Listener = socket;

		m_Thread = std::thread(std::bind(&LivestatusListener::ServerThreadProc, this));

		Log(LogInformation, "LivestatusListener")
			<< "Created UNIX socket in '" << GetSocketPath() << "'.";
#else
		/* no UNIX sockets on windows */
		Log(LogCritical, "LivestatusListener", "Unix sockets are not supported on Windows.");
		return;
#endif
	}
}

void LivestatusListener::Stop(bool runtimeRemoved)
{
	ObjectImpl<LivestatusListener>::Stop(runtimeRemoved);

	Log(LogInformation, "LivestatusListener")
		<< "'" << GetName() << "' stopped.";

	m_Listener->Close();

	if (m_Thread.joinable())
		m_Thread.join();
}

int LivestatusListener::GetClientsConnected()
{
	boost::mutex::scoped_lock lock(l_ComponentMutex);

	return l_ClientsConnected;
}

int LivestatusListener::GetConnections()
{
	boost::mutex::scoped_lock lock(l_ComponentMutex);

	return l_Connections;
}

void LivestatusListener::ServerThreadProc()
{
	m_Listener->Listen();

	try {
		for (;;) {
			timeval tv = { 0, 500000 };

			if (m_Listener->Poll(true, false, &tv)) {
				Socket::Ptr client = m_Listener->Accept();
				Log(LogNotice, "LivestatusListener", "Client connected");
				Utility::QueueAsyncCallback(std::bind(&LivestatusListener::ClientHandler, this, client), LowLatencyScheduler);
			}

			if (!IsActive())
				break;
		}
	} catch (std::exception&) {
		Log(LogCritical, "LivestatusListener", "Cannot accept new connection.");
	}

	m_Listener->Close();
}

void LivestatusListener::ClientHandler(const Socket::Ptr& client)
{
	{
		boost::mutex::scoped_lock lock(l_ComponentMutex);
		l_ClientsConnected++;
		l_Connections++;
	}

	Stream::Ptr stream = new NetworkStream(client);

	StreamReadContext context;

	for (;;) {
		String line;

		std::vector<String> lines;

		for (;;) {
			StreamReadStatus srs = stream->ReadLine(&line, context);

			if (srs == StatusEof)
				break;

			if (srs != StatusNewItem)
				continue;

			if (line.GetLength() > 0)
				lines.push_back(line);
			else
				break;
		}

		if (lines.empty())
			break;

		LivestatusQuery::Ptr query = new LivestatusQuery(lines, GetCompatLogPath());
		if (!query->Execute(stream))
			break;
	}

	{
		boost::mutex::scoped_lock lock(l_ComponentMutex);
		l_ClientsConnected--;
	}
}


void LivestatusListener::ValidateSocketType(const String& value, const ValidationUtils& utils)
{
	ObjectImpl<LivestatusListener>::ValidateSocketType(value, utils);

	if (value != "unix" && value != "tcp")
		BOOST_THROW_EXCEPTION(ValidationError(this, { "socket_type" }, "Socket type '" + value + "' is invalid."));
}
