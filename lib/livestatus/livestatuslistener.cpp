/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/livestatuslistener.hpp"
#include "livestatus/livestatuslistener-ti.cpp"
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
static std::mutex l_ComponentMutex;

REGISTER_STATSFUNCTION(LivestatusListener, &LivestatusListener::StatsFunc);

void LivestatusListener::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	DictionaryData nodes;

	for (const LivestatusListener::Ptr& livestatuslistener : ConfigType::GetObjectsByType<LivestatusListener>()) {
		nodes.emplace_back(livestatuslistener->GetName(), new Dictionary({
			{ "connections", l_Connections }
		}));

		perfdata->Add(new PerfdataValue("livestatuslistener_" + livestatuslistener->GetName() + "_connections", l_Connections));
	}

	status->Set("livestatuslistener", new Dictionary(std::move(nodes)));
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
	std::unique_lock<std::mutex> lock(l_ComponentMutex);

	return l_ClientsConnected;
}

int LivestatusListener::GetConnections()
{
	std::unique_lock<std::mutex> lock(l_ComponentMutex);

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
		std::unique_lock<std::mutex> lock(l_ComponentMutex);
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
		std::unique_lock<std::mutex> lock(l_ComponentMutex);
		l_ClientsConnected--;
	}
}


void LivestatusListener::ValidateSocketType(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<LivestatusListener>::ValidateSocketType(lvalue, utils);

	if (lvalue() != "unix" && lvalue() != "tcp")
		BOOST_THROW_EXCEPTION(ValidationError(this, { "socket_type" }, "Socket type '" + lvalue() + "' is invalid."));
}
