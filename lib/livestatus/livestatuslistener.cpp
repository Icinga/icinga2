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
#include "base/io-engine.hpp"
#include "base/application.hpp"
#include "base/function.hpp"
#include "base/statsfunction.hpp"
#include "base/convert.hpp"
#include <boost/asio/read_until.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/local/stream_protocol.hpp>
#include <string>

using namespace icinga;

REGISTER_TYPE(LivestatusListener);

static int l_ClientsConnected = 0;
static int l_Connections = 0;
static boost::mutex l_ComponentMutex;

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

	auto& io (IoEngine::Get().GetIoService());

	// TCP
	if (GetSocketType() == "tcp") {
		namespace asio = boost::asio;
		namespace ip = asio::ip;
		using ip::tcp;

		String host = GetBindHost();
		String port = GetBindPort();

		std::shared_ptr<tcp::socket> socket(io);

		auto acceptor (std::make_shared<tcp::acceptor>(io));

		try {
			tcp::resolver resolver (io);
			tcp::resolver::query query (host, port, tcp::resolver::query::passive);

			auto result (resolver.resolve(query));
			auto current (result.begin());

			for (;;) {
				try {
					acceptor->open(current->endpoint().protocol());

					{
						auto fd (acceptor->native_handle());

						const int optFalse = 0;
						setsockopt(fd, IPPROTO_IPV6, IPV6_V6ONLY, reinterpret_cast<const char *>(&optFalse), sizeof(optFalse));

						const int optTrue = 1;
						setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&optTrue), sizeof(optTrue));
#ifndef _WIN32
						setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char *>(&optTrue), sizeof(optTrue));
#endif /* _WIN32 */
					}

					acceptor->bind(current->endpoint());

					break;
				} catch (const std::exception&) {
					if (++current == result.end()) {
						throw;
					}

					if (acceptor->is_open()) {
						acceptor->close();
					}
				}
			}

		} catch (const boost::system::system_error& ec) {
			Log(LogCritical, "LivestatusListener")
				<< "Cannot bind TCP socket on host '" << host
				<< "' port '" << port << "': " << ec.what();
			return;
		}

		acceptor->listen(INT_MAX);

		auto localEndpoint (acceptor->local_endpoint());

		Log(LogInformation, "LivestatusListener")
			<< "Started new listener on '[" << localEndpoint.address() << "]:" << localEndpoint.port() << "'";

		// Start listener
		m_Thread = std::thread(std::bind(&LivestatusListener::TcpServerThreadProc, this, acceptor));
	}
	// UNIX
	else if (GetSocketType() == "unix") {
#ifndef _WIN32
		namespace asio = boost::asio;
		namespace local = asio::local;
		using local::stream_protocol;

		String socketPath = GetSocketPath();

		Utility::Remove(socketPath); // Cleanup before

		stream_protocol::endpoint endpoint(socketPath);
		stream_protocol::socket socket(io);
		stream_protocol::acceptor acceptor(io, endpoint);

		try {
			socket.bind(endpoint);
		} catch (const boost::system::system_error& ec) {
			Log(LogCritical, "LivestatusListener")
				<< "Cannot bind UNIX socket to '" << socketPath << "': " << ec.what();
			return;
		}

		/* group must be able to write */
		mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

		if (chmod(socketPath.CStr(), mode) < 0) {
			Log(LogCritical, "LivestatusListener")
				<< "chmod() on unix socket '" << socketPath << "' failed with error code " << errno << ", \""
				<< Utility::FormatErrorNumber(errno) << "\"";
			return;
		}

		// Start listener
		m_Thread = std::thread(std::bind(&LivestatusListener::UnixServerThreadProc, this, acceptor));

		Log(LogInformation, "LivestatusListener")
			<< "Created UNIX socket in '" << socketPath << "'.";
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

void LivestatusListener::TcpServerThreadProc(const std::shared_ptr<boost::asio::ip::tcp::acceptor>& server)
{
	namespace asio = boost::asio;
	namespace ip = asio::ip;
	using ip::tcp;

	auto& io = server->get_executor().context();

	try {
		for (;;) {
			tcp::socket newSocket(io);

			// Throws exception and breaks loop
			server->async_accept([this](boost::system::error_code ec, tcp::socket&& newSocket) {
				LivestatusSocket socket(std::shared_ptr<boost::asio::generic::stream_protocol::socket>(newSocket));

				Log(LogNotice, "LivestatusListener", "Client connected");
				Utility::QueueAsyncCallback(std::bind(&LivestatusListener::ClientHandler, this), LowLatencyScheduler);
			});

			if (!IsActive())
				break;
		}

	} catch (const boost::system::system_error& ec) {
		Log(LogCritical, "LivestatusListener")
			<< "Cannot accept new TCP connection: " << ec.what();
	}

	server->close();
}

void LivestatusListener::UnixServerThreadProc(const std::shared_ptr<boost::asio::local::stream_protocol::acceptor>& server)
{
	namespace asio = boost::asio;
	namespace local = asio::local;
	using local::stream_protocol;

	//TODO
}

/*
void LivestatusListener::ServerThreadProc()
{
	namespace asio = boost::asio;
	namespace ip = asio::ip;
	using ip::tcp;

	auto endpoint = m_Listener.local_endpoint();
	auto& io = m_Listener.get_executor().context();

	if (GetSocketType() == "tcp") {

	}
	else if (GetSocketType() == "unix") {
		asio::local::stream_protocol::acceptor acceptor(io, endpoint);
	}


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

	m_Listener.shutdown(tcp::socket::shutdown_both);
	m_Listener.close();

}
 */

void LivestatusListener::ClientHandler(const std::shared_ptr<boost::asio::generic::stream_protocol::socket>& socket)
{
	namespace asio = boost::asio;

	{
		boost::mutex::scoped_lock lock(l_ComponentMutex);
		l_ClientsConnected++;
		l_Connections++;
	}

	for (;;) {
		std::vector<String> lines;

		for (;;) {
			asio::streambuf buf;

			try {
				asio::read_until(socket, buf, "\r\n");
			} catch (const boost::system::system_error& ec) {
				break;
			}

			std::istream is(&buf);
			std::string l;

			std::getline(is, l);

			String line = l;

			if (line.GetLength() > 0)
				lines.push_back(line);
			else
				break;
		}

		if (lines.empty())
			break;

		LivestatusQuery::Ptr query = new LivestatusQuery(lines, GetCompatLogPath());
		if (!query->Execute(socket))
			break;
	}

	{
		boost::mutex::scoped_lock lock(l_ComponentMutex);
		l_ClientsConnected--;
	}
}


void LivestatusListener::ValidateSocketType(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<LivestatusListener>::ValidateSocketType(lvalue, utils);

	if (lvalue() != "unix" && lvalue() != "tcp")
		BOOST_THROW_EXCEPTION(ValidationError(this, { "socket_type" }, "Socket type '" + lvalue() + "' is invalid."));
}
