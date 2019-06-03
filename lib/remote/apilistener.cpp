/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/apilistener.hpp"
#include "remote/apilistener-ti.cpp"
#include "remote/jsonrpcconnection.hpp"
#include "remote/endpoint.hpp"
#include "remote/jsonrpc.hpp"
#include "remote/apifunction.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/configobjectutility.hpp"
#include "base/convert.hpp"
#include "base/defer.hpp"
#include "base/io-engine.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/stdiostream.hpp"
#include "base/perfdatavalue.hpp"
#include "base/application.hpp"
#include "base/context.hpp"
#include "base/statsfunction.hpp"
#include "base/exception.hpp"
#include "base/tcpsocket.hpp"
#include <boost/asio/buffer.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/system/error_code.hpp>
#include <climits>
#include <fstream>
#include <memory>
#include <openssl/ssl.h>
#include <openssl/tls1.h>
#include <openssl/x509.h>
#include <sstream>
#include <utility>

using namespace icinga;

REGISTER_TYPE(ApiListener);

boost::signals2::signal<void(bool)> ApiListener::OnMasterChanged;
ApiListener::Ptr ApiListener::m_Instance;

REGISTER_STATSFUNCTION(ApiListener, &ApiListener::StatsFunc);

REGISTER_APIFUNCTION(Hello, icinga, &ApiListener::HelloAPIHandler);

ApiListener::ApiListener()
{
	m_RelayQueue.SetName("ApiListener, RelayQueue");
	m_SyncQueue.SetName("ApiListener, SyncQueue");
}

String ApiListener::GetApiDir()
{
	return Configuration::DataDir + "/api/";
}

String ApiListener::GetCertsDir()
{
	return Configuration::DataDir + "/certs/";
}

String ApiListener::GetCaDir()
{
	return Configuration::DataDir + "/ca/";
}

String ApiListener::GetCertificateRequestsDir()
{
	return Configuration::DataDir + "/certificate-requests/";
}

String ApiListener::GetDefaultCertPath()
{
	return GetCertsDir() + "/" + ScriptGlobal::Get("NodeName") + ".crt";
}

String ApiListener::GetDefaultKeyPath()
{
	return GetCertsDir() + "/" + ScriptGlobal::Get("NodeName") + ".key";
}

String ApiListener::GetDefaultCaPath()
{
	return GetCertsDir() + "/ca.crt";
}

double ApiListener::GetTlsHandshakeTimeout() const
{
	return Configuration::TlsHandshakeTimeout;
}

void ApiListener::SetTlsHandshakeTimeout(double value, bool suppress_events, const Value& cookie)
{
	Configuration::TlsHandshakeTimeout = value;
}

void ApiListener::CopyCertificateFile(const String& oldCertPath, const String& newCertPath)
{
	struct stat st1, st2;

	if (!oldCertPath.IsEmpty() && stat(oldCertPath.CStr(), &st1) >= 0 && (stat(newCertPath.CStr(), &st2) < 0 || st1.st_mtime > st2.st_mtime)) {
		Log(LogWarning, "ApiListener")
			<< "Copying '" << oldCertPath << "' certificate file to '" << newCertPath << "'";

		Utility::MkDirP(Utility::DirName(newCertPath), 0700);
		Utility::CopyFile(oldCertPath, newCertPath);
	}
}

void ApiListener::OnConfigLoaded()
{
	if (m_Instance)
		BOOST_THROW_EXCEPTION(ScriptError("Only one ApiListener object is allowed.", GetDebugInfo()));

	m_Instance = this;

	String defaultCertPath = GetDefaultCertPath();
	String defaultKeyPath = GetDefaultKeyPath();
	String defaultCaPath = GetDefaultCaPath();

	/* Migrate certificate location < 2.8 to the new default path. */
	String oldCertPath = GetCertPath();
	String oldKeyPath = GetKeyPath();
	String oldCaPath = GetCaPath();

	CopyCertificateFile(oldCertPath, defaultCertPath);
	CopyCertificateFile(oldKeyPath, defaultKeyPath);
	CopyCertificateFile(oldCaPath, defaultCaPath);

	if (!oldCertPath.IsEmpty() && !oldKeyPath.IsEmpty() && !oldCaPath.IsEmpty()) {
		Log(LogWarning, "ApiListener", "Please read the upgrading documentation for v2.8: https://icinga.com/docs/icinga2/latest/doc/16-upgrading-icinga-2/");
	}

	/* Create the internal API object storage. */
	ConfigObjectUtility::CreateStorage();

	/* Cache API packages and their active stage name. */
	UpdateActivePackageStagesCache();

	/* set up SSL context */
	std::shared_ptr<X509> cert;
	try {
		cert = GetX509Certificate(defaultCertPath);
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ScriptError("Cannot get certificate from cert path: '"
			+ defaultCertPath + "'.", GetDebugInfo()));
	}

	try {
		SetIdentity(GetCertificateCN(cert));
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ScriptError("Cannot get certificate common name from cert path: '"
			+ defaultCertPath + "'.", GetDebugInfo()));
	}

	Log(LogInformation, "ApiListener")
		<< "My API identity: " << GetIdentity();

	UpdateSSLContext();
}

void ApiListener::UpdateSSLContext()
{
	namespace ssl = boost::asio::ssl;

	std::shared_ptr<ssl::context> context;

	try {
		context = MakeAsioSslContext(GetDefaultCertPath(), GetDefaultKeyPath(), GetDefaultCaPath());
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ScriptError("Cannot make SSL context for cert path: '"
			+ GetDefaultCertPath() + "' key path: '" + GetDefaultKeyPath() + "' ca path: '" + GetDefaultCaPath() + "'.", GetDebugInfo()));
	}

	if (!GetCrlPath().IsEmpty()) {
		try {
			AddCRLToSSLContext(context, GetCrlPath());
		} catch (const std::exception&) {
			BOOST_THROW_EXCEPTION(ScriptError("Cannot add certificate revocation list to SSL context for crl path: '"
				+ GetCrlPath() + "'.", GetDebugInfo()));
		}
	}

	if (!GetCipherList().IsEmpty()) {
		try {
			SetCipherListToSSLContext(context, GetCipherList());
		} catch (const std::exception&) {
			BOOST_THROW_EXCEPTION(ScriptError("Cannot set cipher list to SSL context for cipher list: '"
				+ GetCipherList() + "'.", GetDebugInfo()));
		}
	}

	if (!GetTlsProtocolmin().IsEmpty()){
		try {
			SetTlsProtocolminToSSLContext(context, GetTlsProtocolmin());
		} catch (const std::exception&) {
			BOOST_THROW_EXCEPTION(ScriptError("Cannot set minimum TLS protocol version to SSL context with tls_protocolmin: '" + GetTlsProtocolmin() + "'.", GetDebugInfo()));
		}
	}

	m_SSLContext = context;

	for (const Endpoint::Ptr& endpoint : ConfigType::GetObjectsByType<Endpoint>()) {
		for (const JsonRpcConnection::Ptr& client : endpoint->GetClients()) {
			client->Disconnect();
		}
	}

	for (const JsonRpcConnection::Ptr& client : m_AnonymousClients) {
		client->Disconnect();
	}
}

void ApiListener::OnAllConfigLoaded()
{
	m_LocalEndpoint = Endpoint::GetByName(GetIdentity());

	if (!m_LocalEndpoint)
		BOOST_THROW_EXCEPTION(ScriptError("Endpoint object for '" + GetIdentity() + "' is missing.", GetDebugInfo()));
}

/**
 * Starts the component.
 */
void ApiListener::Start(bool runtimeCreated)
{
	Log(LogInformation, "ApiListener")
		<< "'" << GetName() << "' started.";

	SyncZoneDirs();

	ObjectImpl<ApiListener>::Start(runtimeCreated);

	{
		boost::mutex::scoped_lock lock(m_LogLock);
		OpenLogFile();
	}

	/* create the primary JSON-RPC listener */
	if (!AddListener(GetBindHost(), GetBindPort())) {
		Log(LogCritical, "ApiListener")
			<< "Cannot add listener on host '" << GetBindHost() << "' for port '" << GetBindPort() << "'.";
		Application::Exit(EXIT_FAILURE);
	}

	m_Timer = new Timer();
	m_Timer->OnTimerExpired.connect(std::bind(&ApiListener::ApiTimerHandler, this));
	m_Timer->SetInterval(5);
	m_Timer->Start();
	m_Timer->Reschedule(0);

	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->OnTimerExpired.connect(std::bind(&ApiListener::ApiReconnectTimerHandler, this));
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	/* Keep this in relative sync with the cold startup in UpdateObjectAuthority() and the reconnect interval above.
	 * Previous: 60s reconnect, 30s OA, 60s cold startup.
	 * Now: 10s reconnect, 10s OA, 30s cold startup. 
	 */
	m_AuthorityTimer = new Timer();
	m_AuthorityTimer->OnTimerExpired.connect(std::bind(&ApiListener::UpdateObjectAuthority));
	m_AuthorityTimer->SetInterval(10);
	m_AuthorityTimer->Start();

	m_CleanupCertificateRequestsTimer = new Timer();
	m_CleanupCertificateRequestsTimer->OnTimerExpired.connect(std::bind(&ApiListener::CleanupCertificateRequestsTimerHandler, this));
	m_CleanupCertificateRequestsTimer->SetInterval(3600);
	m_CleanupCertificateRequestsTimer->Start();
	m_CleanupCertificateRequestsTimer->Reschedule(0);

	m_ApiPackageIntegrityTimer = new Timer();
	m_ApiPackageIntegrityTimer->OnTimerExpired.connect(std::bind(&ApiListener::CheckApiPackageIntegrity, this));
	m_ApiPackageIntegrityTimer->SetInterval(300);
	m_ApiPackageIntegrityTimer->Start();

	OnMasterChanged(true);
}

void ApiListener::Stop(bool runtimeDeleted)
{
	ObjectImpl<ApiListener>::Stop(runtimeDeleted);

	Log(LogInformation, "ApiListener")
		<< "'" << GetName() << "' stopped.";

	{
		boost::mutex::scoped_lock lock(m_LogLock);
		CloseLogFile();
		RotateLogFile();
	}

	RemoveStatusFile();
}

ApiListener::Ptr ApiListener::GetInstance()
{
	return m_Instance;
}

Endpoint::Ptr ApiListener::GetMaster() const
{
	Zone::Ptr zone = Zone::GetLocalZone();

	if (!zone)
		return nullptr;

	std::vector<String> names;

	for (const Endpoint::Ptr& endpoint : zone->GetEndpoints())
		if (endpoint->GetConnected() || endpoint->GetName() == GetIdentity())
			names.push_back(endpoint->GetName());

	std::sort(names.begin(), names.end());

	return Endpoint::GetByName(*names.begin());
}

bool ApiListener::IsMaster() const
{
	Endpoint::Ptr master = GetMaster();

	if (!master)
		return false;

	return master == GetLocalEndpoint();
}

/**
 * Creates a new JSON-RPC listener on the specified port.
 *
 * @param node The host the listener should be bound to.
 * @param service The port to listen on.
 */
bool ApiListener::AddListener(const String& node, const String& service)
{
	namespace asio = boost::asio;
	namespace ip = asio::ip;
	using ip::tcp;

	ObjectLock olock(this);

	auto sslContext (m_SSLContext);

	if (!sslContext) {
		Log(LogCritical, "ApiListener", "SSL context is required for AddListener()");
		return false;
	}

	auto& io (IoEngine::Get().GetIoService());
	auto acceptor (std::make_shared<tcp::acceptor>(io));

	try {
		tcp::resolver resolver (io);
		tcp::resolver::query query (node, service, tcp::resolver::query::passive);

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
	} catch (const std::exception& ex) {
		Log(LogCritical, "ApiListener")
			<< "Cannot bind TCP socket for host '" << node << "' on port '" << service << "': " << DiagnosticInformation(ex, false);
		return false;
	}

	acceptor->listen(INT_MAX);

	auto localEndpoint (acceptor->local_endpoint());

	Log(LogInformation, "ApiListener")
		<< "Started new listener on '[" << localEndpoint.address() << "]:" << localEndpoint.port() << "'";

	asio::spawn(io, [this, acceptor, sslContext](asio::yield_context yc) { ListenerCoroutineProc(yc, acceptor, sslContext); });

	UpdateStatusFile(localEndpoint);

	return true;
}

void ApiListener::ListenerCoroutineProc(boost::asio::yield_context yc, const std::shared_ptr<boost::asio::ip::tcp::acceptor>& server, const std::shared_ptr<boost::asio::ssl::context>& sslContext)
{
	namespace asio = boost::asio;

	auto& io (server->get_executor().context());

	for (;;) {
		try {
			auto sslConn (std::make_shared<AsioTlsStream>(io, *sslContext));

			server->async_accept(sslConn->lowest_layer(), yc);

			asio::spawn(io, [this, sslConn](asio::yield_context yc) { NewClientHandler(yc, sslConn, String(), RoleServer); });
		} catch (const std::exception& ex) {
			Log(LogCritical, "ApiListener")
				<< "Cannot accept new connection: " << DiagnosticInformation(ex, false);
		}
	}
}

/**
 * Creates a new JSON-RPC client and connects to the specified endpoint.
 *
 * @param endpoint The endpoint.
 */
void ApiListener::AddConnection(const Endpoint::Ptr& endpoint)
{
	namespace asio = boost::asio;
	using asio::ip::tcp;

	auto sslContext (m_SSLContext);

	if (!sslContext) {
		Log(LogCritical, "ApiListener", "SSL context is required for AddConnection()");
		return;
	}

	auto& io (IoEngine::Get().GetIoService());

	asio::spawn(io, [this, endpoint, &io, sslContext](asio::yield_context yc) {
		String host = endpoint->GetHost();
		String port = endpoint->GetPort();

		Log(LogInformation, "ApiListener")
			<< "Reconnecting to endpoint '" << endpoint->GetName() << "' via host '" << host << "' and port '" << port << "'";

		try {
			auto sslConn (std::make_shared<AsioTlsStream>(io, *sslContext, endpoint->GetName()));

			Connect(sslConn->lowest_layer(), host, port, yc);

			NewClientHandler(yc, sslConn, endpoint->GetName(), RoleClient);

			endpoint->SetConnecting(false);
			Log(LogInformation, "ApiListener")
				<< "Finished reconnecting to endpoint '" << endpoint->GetName() << "' via host '" << host << "' and port '" << port << "'";
		} catch (const std::exception& ex) {
			endpoint->SetConnecting(false);

			Log(LogCritical, "ApiListener")
				<< "Cannot connect to host '" << host << "' on port '" << port << "': " << ex.what();
		}
	});
}

void ApiListener::NewClientHandler(boost::asio::yield_context yc, const std::shared_ptr<AsioTlsStream>& client, const String& hostname, ConnectionRole role)
{
	try {
		NewClientHandlerInternal(yc, client, hostname, role);
	} catch (const std::exception& ex) {
		Log(LogCritical, "ApiListener")
			<< "Exception while handling new API client connection: " << DiagnosticInformation(ex, false);

		Log(LogDebug, "ApiListener")
			<< "Exception while handling new API client connection: " << DiagnosticInformation(ex);
	}
}

/**
 * Processes a new client connection.
 *
 * @param client The new client.
 */
void ApiListener::NewClientHandlerInternal(boost::asio::yield_context yc, const std::shared_ptr<AsioTlsStream>& client, const String& hostname, ConnectionRole role)
{
	namespace asio = boost::asio;
	namespace ssl = asio::ssl;

	String conninfo;

	{
		std::ostringstream conninfo_;

		if (role == RoleClient) {
			conninfo_ << "to";
		} else {
			conninfo_ << "from";
		}

		auto endpoint (client->lowest_layer().remote_endpoint());

		conninfo_ << " [" << endpoint.address() << "]:" << endpoint.port();

		conninfo = conninfo_.str();
	}

	auto& sslConn (client->next_layer());

	try {
		sslConn.async_handshake(role == RoleClient ? sslConn.client : sslConn.server, yc);
	} catch (const std::exception& ex) {
		Log(LogCritical, "ApiListener")
			<< "Client TLS handshake failed (" << conninfo << "): " << DiagnosticInformation(ex, false);
		return;
	}

	bool willBeShutDown = false;

	Defer shutDownIfNeeded ([&sslConn, &willBeShutDown, &yc]() {
		if (!willBeShutDown) {
			sslConn.async_shutdown(yc);
		}
	});

	std::shared_ptr<X509> cert (sslConn.GetPeerCertificate());
	bool verify_ok = false;
	String identity;
	Endpoint::Ptr endpoint;

	if (cert) {
		verify_ok = sslConn.IsVerifyOK();

		String verifyError = sslConn.GetVerifyError();

		try {
			identity = GetCertificateCN(cert);
		} catch (const std::exception&) {
			Log(LogCritical, "ApiListener")
				<< "Cannot get certificate common name from cert path: '" << GetDefaultCertPath() << "'.";
			return;
		}

		if (!hostname.IsEmpty()) {
			if (identity != hostname) {
				Log(LogWarning, "ApiListener")
					<< "Unexpected certificate common name while connecting to endpoint '"
					<< hostname << "': got '" << identity << "'";
				return;
			} else if (!verify_ok) {
				Log(LogWarning, "ApiListener")
					<< "Certificate validation failed for endpoint '" << hostname
					<< "': " << verifyError;
			}
		}

		if (verify_ok) {
			endpoint = Endpoint::GetByName(identity);
		}

		Log log(LogInformation, "ApiListener");

		log << "New client connection for identity '" << identity << "' " << conninfo;

		if (!verify_ok) {
			log << " (certificate validation failed: " << verifyError << ")";
		} else if (!endpoint) {
			log << " (no Endpoint object found for identity)";
		}
	} else {
		Log(LogInformation, "ApiListener")
			<< "New client connection " << conninfo << " (no client certificate)";
	}

	ClientType ctype;

	if (role == RoleClient) {
		JsonRpc::SendMessage(client, new Dictionary({
			{ "jsonrpc", "2.0" },
			{ "method", "icinga::Hello" },
			{ "params", new Dictionary() }
		}), yc);

		client->async_flush(yc);

		ctype = ClientJsonRpc;
	} else {
		{
			boost::system::error_code ec;

			if (client->async_fill(yc[ec]) == 0u) {
				if (identity.IsEmpty()) {
					Log(LogInformation, "ApiListener")
						<< "No data received on new API connection. "
						<< "Ensure that the remote endpoints are properly configured in a cluster setup.";
				} else {
					Log(LogWarning, "ApiListener")
						<< "No data received on new API connection for identity '" << identity << "'. "
						<< "Ensure that the remote endpoints are properly configured in a cluster setup.";
				}

				return;
			}
		}

		char firstByte = 0;

		{
			asio::mutable_buffer firstByteBuf (&firstByte, 1);
			client->peek(firstByteBuf);
		}

		if (firstByte >= '0' && firstByte <= '9') {
			ctype = ClientJsonRpc;
		} else {
			ctype = ClientHttp;
		}
	}

	if (ctype == ClientJsonRpc) {
		Log(LogNotice, "ApiListener", "New JSON-RPC client");

		JsonRpcConnection::Ptr aclient = new JsonRpcConnection(identity, verify_ok, client, role);

		if (endpoint) {
			bool needSync = !endpoint->GetConnected();

			endpoint->AddClient(aclient);

			asio::spawn(client->get_executor().context(), [this, aclient, endpoint, needSync](asio::yield_context yc) {
				CpuBoundWork syncClient (yc);

				SyncClient(aclient, endpoint, needSync);
			});
		} else if (!AddAnonymousClient(aclient)) {
			Log(LogNotice, "ApiListener")
				<< "Ignoring anonymous JSON-RPC connection " << conninfo
				<< ". Max connections (" << GetMaxAnonymousClients() << ") exceeded.";

			aclient = nullptr;
		}

		if (aclient) {
			aclient->Start();

			willBeShutDown = true;
		}
	} else {
		Log(LogNotice, "ApiListener", "New HTTP client");

		HttpServerConnection::Ptr aclient = new HttpServerConnection(identity, verify_ok, client);
		AddHttpClient(aclient);
		aclient->Start();

		willBeShutDown = true;
	}
}

void ApiListener::SyncClient(const JsonRpcConnection::Ptr& aclient, const Endpoint::Ptr& endpoint, bool needSync)
{
	Zone::Ptr eZone = endpoint->GetZone();

	try {
		{
			ObjectLock olock(endpoint);

			endpoint->SetSyncing(true);
		}

		Zone::Ptr myZone = Zone::GetLocalZone();

		if (myZone->GetParent() == eZone) {
			Log(LogInformation, "ApiListener")
				<< "Requesting new certificate for this Icinga instance from endpoint '" << endpoint->GetName() << "'.";

			JsonRpcConnection::SendCertificateRequest(aclient, nullptr, String());

			if (Utility::PathExists(ApiListener::GetCertificateRequestsDir()))
				Utility::Glob(ApiListener::GetCertificateRequestsDir() + "/*.json", std::bind(&JsonRpcConnection::SendCertificateRequest, aclient, nullptr, _1), GlobFile);
		}

		/* Make sure that the config updates are synced
		 * before the logs are replayed.
		 */

		Log(LogInformation, "ApiListener")
			<< "Sending config updates for endpoint '" << endpoint->GetName() << "' in zone '" << eZone->GetName() << "'.";

		/* sync zone file config */
		SendConfigUpdate(aclient);

		Log(LogInformation, "ApiListener")
			<< "Finished sending config file updates for endpoint '" << endpoint->GetName() << "' in zone '" << eZone->GetName() << "'.";

		/* sync runtime config */
		SendRuntimeConfigObjects(aclient);

		Log(LogInformation, "ApiListener")
			<< "Finished sending runtime config updates for endpoint '" << endpoint->GetName() << "' in zone '" << eZone->GetName() << "'.";

		if (!needSync) {
			ObjectLock olock2(endpoint);
			endpoint->SetSyncing(false);
			return;
		}

		Log(LogInformation, "ApiListener")
			<< "Sending replay log for endpoint '" << endpoint->GetName() << "' in zone '" << eZone->GetName() << "'.";

		ReplayLog(aclient);

		if (eZone == Zone::GetLocalZone())
			UpdateObjectAuthority();

		Log(LogInformation, "ApiListener")
			<< "Finished sending replay log for endpoint '" << endpoint->GetName() << "' in zone '" << eZone->GetName() << "'.";
	} catch (const std::exception& ex) {
		{
			ObjectLock olock2(endpoint);
			endpoint->SetSyncing(false);
		}

		Log(LogCritical, "ApiListener")
			<< "Error while syncing endpoint '" << endpoint->GetName() << "': " << DiagnosticInformation(ex, false);

		Log(LogDebug, "ApiListener")
			<< "Error while syncing endpoint '" << endpoint->GetName() << "': " << DiagnosticInformation(ex);
	}

	Log(LogInformation, "ApiListener")
		<< "Finished syncing endpoint '" << endpoint->GetName() << "' in zone '" << eZone->GetName() << "'.";
}

void ApiListener::ApiTimerHandler()
{
	double now = Utility::GetTime();

	std::vector<int> files;
	Utility::Glob(GetApiDir() + "log/*", std::bind(&ApiListener::LogGlobHandler, std::ref(files), _1), GlobFile);
	std::sort(files.begin(), files.end());

	for (int ts : files) {
		bool need = false;
		auto localZone (GetLocalEndpoint()->GetZone());

		for (const Endpoint::Ptr& endpoint : ConfigType::GetObjectsByType<Endpoint>()) {
			if (endpoint == GetLocalEndpoint())
				continue;

			auto zone (endpoint->GetZone());

			/* only care for endpoints in a) the same zone b) our parent zone c) immediate child zones */
			if (!(zone == localZone || zone == localZone->GetParent() || zone->GetParent() == localZone)) {
				continue;
			}

			if (endpoint->GetLogDuration() >= 0 && ts < now - endpoint->GetLogDuration())
				continue;

			if (ts > endpoint->GetLocalLogPosition()) {
				need = true;
				break;
			}
		}

		if (!need) {
			String path = GetApiDir() + "log/" + Convert::ToString(ts);
			Log(LogNotice, "ApiListener")
				<< "Removing old log file: " << path;
			(void)unlink(path.CStr());
		}
	}

	for (const Endpoint::Ptr& endpoint : ConfigType::GetObjectsByType<Endpoint>()) {
		if (!endpoint->GetConnected())
			continue;

		double ts = endpoint->GetRemoteLogPosition();

		if (ts == 0)
			continue;

		Dictionary::Ptr lmessage = new Dictionary({
			{ "jsonrpc", "2.0" },
			{ "method", "log::SetLogPosition" },
			{ "params", new Dictionary({
				{ "log_position", ts }
			}) }
		});

		double maxTs = 0;

		for (const JsonRpcConnection::Ptr& client : endpoint->GetClients()) {
			if (client->GetTimestamp() > maxTs)
				maxTs = client->GetTimestamp();
		}

		for (const JsonRpcConnection::Ptr& client : endpoint->GetClients()) {
			if (client->GetTimestamp() == maxTs) {
				client->SendMessage(lmessage);
			} else {
				client->Disconnect();
			}
		}

		Log(LogNotice, "ApiListener")
			<< "Setting log position for identity '" << endpoint->GetName() << "': "
			<< Utility::FormatDateTime("%Y/%m/%d %H:%M:%S", ts);
	}
}

void ApiListener::ApiReconnectTimerHandler()
{
	Zone::Ptr my_zone = Zone::GetLocalZone();

	for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
		/* don't connect to global zones */
		if (zone->GetGlobal())
			continue;

		/* only connect to endpoints in a) the same zone b) our parent zone c) immediate child zones */
		if (my_zone != zone && my_zone != zone->GetParent() && zone != my_zone->GetParent()) {
			Log(LogDebug, "ApiListener")
				<< "Not connecting to Zone '" << zone->GetName()
				<< "' because it's not in the same zone, a parent or a child zone.";
			continue;
		}

		for (const Endpoint::Ptr& endpoint : zone->GetEndpoints()) {
			/* don't connect to ourselves */
			if (endpoint == GetLocalEndpoint()) {
				Log(LogDebug, "ApiListener")
					<< "Not connecting to Endpoint '" << endpoint->GetName() << "' because that's us.";
				continue;
			}

			/* don't try to connect to endpoints which don't have a host and port */
			if (endpoint->GetHost().IsEmpty() || endpoint->GetPort().IsEmpty()) {
				Log(LogDebug, "ApiListener")
					<< "Not connecting to Endpoint '" << endpoint->GetName()
					<< "' because the host/port attributes are missing.";
				continue;
			}

			/* don't try to connect if there's already a connection attempt */
			if (endpoint->GetConnecting()) {
				Log(LogDebug, "ApiListener")
					<< "Not connecting to Endpoint '" << endpoint->GetName()
					<< "' because we're already trying to connect to it.";
				continue;
			}

			/* don't try to connect if we're already connected */
			if (endpoint->GetConnected()) {
				Log(LogDebug, "ApiListener")
					<< "Not connecting to Endpoint '" << endpoint->GetName()
					<< "' because we're already connected to it.";
				continue;
			}

			/* Set connecting state to prevent duplicated queue inserts later. */
			endpoint->SetConnecting(true);

			AddConnection(endpoint);
		}
	}

	Endpoint::Ptr master = GetMaster();

	if (master)
		Log(LogNotice, "ApiListener")
			<< "Current zone master: " << master->GetName();

	std::vector<String> names;
	for (const Endpoint::Ptr& endpoint : ConfigType::GetObjectsByType<Endpoint>())
		if (endpoint->GetConnected())
			names.emplace_back(endpoint->GetName() + " (" + Convert::ToString(endpoint->GetClients().size()) + ")");

	Log(LogNotice, "ApiListener")
		<< "Connected endpoints: " << Utility::NaturalJoin(names);
}

static void CleanupCertificateRequest(const String& path, double expiryTime)
{
#ifndef _WIN32
	struct stat statbuf;
	if (lstat(path.CStr(), &statbuf) < 0)
		return;
#else /* _WIN32 */
	struct _stat statbuf;
	if (_stat(path.CStr(), &statbuf) < 0)
		return;
#endif /* _WIN32 */

	if (statbuf.st_mtime < expiryTime)
		(void) unlink(path.CStr());
}

void ApiListener::CleanupCertificateRequestsTimerHandler()
{
	String requestsDir = GetCertificateRequestsDir();

	if (Utility::PathExists(requestsDir)) {
		/* remove certificate requests that are older than a week */
		double expiryTime = Utility::GetTime() - 7 * 24 * 60 * 60;
		Utility::Glob(requestsDir + "/*.json", std::bind(&CleanupCertificateRequest, _1, expiryTime), GlobFile);
	}
}

void ApiListener::RelayMessage(const MessageOrigin::Ptr& origin,
	const ConfigObject::Ptr& secobj, const Dictionary::Ptr& message, bool log)
{
	if (!IsActive())
		return;

	m_RelayQueue.Enqueue(std::bind(&ApiListener::SyncRelayMessage, this, origin, secobj, message, log), PriorityNormal, true);
}

void ApiListener::PersistMessage(const Dictionary::Ptr& message, const ConfigObject::Ptr& secobj)
{
	double ts = message->Get("ts");

	ASSERT(ts != 0);

	Dictionary::Ptr pmessage = new Dictionary();
	pmessage->Set("timestamp", ts);

	pmessage->Set("message", JsonEncode(message));

	if (secobj) {
		Dictionary::Ptr secname = new Dictionary();
		secname->Set("type", secobj->GetReflectionType()->GetName());
		secname->Set("name", secobj->GetName());
		pmessage->Set("secobj", secname);
	}

	boost::mutex::scoped_lock lock(m_LogLock);
	if (m_LogFile) {
		NetString::WriteStringToStream(m_LogFile, JsonEncode(pmessage));
		m_LogMessageCount++;
		SetLogMessageTimestamp(ts);

		if (m_LogMessageCount > 50000) {
			CloseLogFile();
			RotateLogFile();
			OpenLogFile();
		}
	}
}

void ApiListener::SyncSendMessage(const Endpoint::Ptr& endpoint, const Dictionary::Ptr& message)
{
	ObjectLock olock(endpoint);

	if (!endpoint->GetSyncing()) {
		Log(LogNotice, "ApiListener")
			<< "Sending message '" << message->Get("method") << "' to '" << endpoint->GetName() << "'";

		double maxTs = 0;

		for (const JsonRpcConnection::Ptr& client : endpoint->GetClients()) {
			if (client->GetTimestamp() > maxTs)
				maxTs = client->GetTimestamp();
		}

		for (const JsonRpcConnection::Ptr& client : endpoint->GetClients()) {
			if (client->GetTimestamp() != maxTs)
				continue;

			client->SendMessage(message);
		}
	}
}

bool ApiListener::RelayMessageOne(const Zone::Ptr& targetZone, const MessageOrigin::Ptr& origin, const Dictionary::Ptr& message, const Endpoint::Ptr& currentZoneMaster)
{
	ASSERT(targetZone);

	Zone::Ptr localZone = Zone::GetLocalZone();

	/* only relay the message to a) the same local zone, b) the parent zone and c) direct child zones. Exception is a global zone. */
	if (!targetZone->GetGlobal() &&
		targetZone != localZone &&
		targetZone != localZone->GetParent() &&
		targetZone->GetParent() != localZone) {
		return true;
	}

	Endpoint::Ptr localEndpoint = GetLocalEndpoint();

	std::vector<Endpoint::Ptr> skippedEndpoints;

	bool relayed = false, log_needed = false, log_done = false;

	std::set<Endpoint::Ptr> targetEndpoints;

	if (targetZone->GetGlobal()) {
		targetEndpoints = localZone->GetEndpoints();

		for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
			/* Fetch immediate child zone members */
			if (zone->GetParent() == localZone) {
				std::set<Endpoint::Ptr> endpoints = zone->GetEndpoints();
				targetEndpoints.insert(endpoints.begin(), endpoints.end());
			}
		}
	} else {
		targetEndpoints = targetZone->GetEndpoints();
	}

	for (const Endpoint::Ptr& targetEndpoint : targetEndpoints) {
		/* Don't relay messages to ourselves. */
		if (targetEndpoint == localEndpoint)
			continue;

		log_needed = true;

		/* Don't relay messages to disconnected endpoints. */
		if (!targetEndpoint->GetConnected()) {
			if (targetZone == localZone)
				log_done = false;

			continue;
		}

		log_done = true;

		/* Don't relay the message to the zone through more than one endpoint unless this is our own zone.
		 * 'relayed' is set to true on success below, enabling the checks in the second iteration.
		 */
		if (relayed && targetZone != localZone) {
			skippedEndpoints.push_back(targetEndpoint);
			continue;
		}

		/* Don't relay messages back to the endpoint which we got the message from. */
		if (origin && origin->FromClient && targetEndpoint == origin->FromClient->GetEndpoint()) {
			skippedEndpoints.push_back(targetEndpoint);
			continue;
		}

		/* Don't relay messages back to the zone which we got the message from. */
		if (origin && origin->FromZone && targetZone == origin->FromZone) {
			skippedEndpoints.push_back(targetEndpoint);
			continue;
		}

		/* Only relay message to the zone master if we're not currently the zone master.
		 * e1 is zone master, e2 and e3 are zone members.
		 *
		 * Message is sent from e2 or e3:
		 *   !isMaster == true
		 *   targetEndpoint e1 is zone master -> send the message
		 *   targetEndpoint e3 is not zone master -> skip it, avoid routing loops
		 *
		 * Message is sent from e1:
		 *   !isMaster == false -> send the messages to e2 and e3 being the zone routing master.
		 */
		bool isMaster = (currentZoneMaster == localEndpoint);

		if (!isMaster && targetEndpoint != currentZoneMaster) {
			skippedEndpoints.push_back(targetEndpoint);
			continue;
		}

		relayed = true;

		SyncSendMessage(targetEndpoint, message);
	}

	if (!skippedEndpoints.empty()) {
		double ts = message->Get("ts");

		for (const Endpoint::Ptr& skippedEndpoint : skippedEndpoints)
			skippedEndpoint->SetLocalLogPosition(ts);
	}

	return !log_needed || log_done;
}

void ApiListener::SyncRelayMessage(const MessageOrigin::Ptr& origin,
	const ConfigObject::Ptr& secobj, const Dictionary::Ptr& message, bool log)
{
	double ts = Utility::GetTime();
	message->Set("ts", ts);

	Log(LogNotice, "ApiListener")
		<< "Relaying '" << message->Get("method") << "' message";

	if (origin && origin->FromZone)
		message->Set("originZone", origin->FromZone->GetName());

	Zone::Ptr target_zone;

	if (secobj) {
		if (secobj->GetReflectionType() == Zone::TypeInstance)
			target_zone = static_pointer_cast<Zone>(secobj);
		else
			target_zone = static_pointer_cast<Zone>(secobj->GetZone());
	}

	if (!target_zone)
		target_zone = Zone::GetLocalZone();

	Endpoint::Ptr master = GetMaster();

	bool need_log = !RelayMessageOne(target_zone, origin, message, master);

	for (const Zone::Ptr& zone : target_zone->GetAllParentsRaw()) {
		if (!RelayMessageOne(zone, origin, message, master))
			need_log = true;
	}

	if (log && need_log)
		PersistMessage(message, secobj);
}

/* must hold m_LogLock */
void ApiListener::OpenLogFile()
{
	String path = GetApiDir() + "log/current";

	Utility::MkDirP(Utility::DirName(path), 0750);

	auto *fp = new std::fstream(path.CStr(), std::fstream::out | std::ofstream::app);

	if (!fp->good()) {
		Log(LogWarning, "ApiListener")
			<< "Could not open spool file: " << path;
		return;
	}

	m_LogFile = new StdioStream(fp, true);
	m_LogMessageCount = 0;
	SetLogMessageTimestamp(Utility::GetTime());
}

/* must hold m_LogLock */
void ApiListener::CloseLogFile()
{
	if (!m_LogFile)
		return;

	m_LogFile->Close();
	m_LogFile.reset();
}

/* must hold m_LogLock */
void ApiListener::RotateLogFile()
{
	double ts = GetLogMessageTimestamp();

	if (ts == 0)
		ts = Utility::GetTime();

	String oldpath = GetApiDir() + "log/current";
	String newpath = GetApiDir() + "log/" + Convert::ToString(static_cast<int>(ts)+1);

	// If the log is being rotated more than once per second,
	// don't overwrite the previous one, but silently deny rotation.
	if (!Utility::PathExists(newpath)) {
		(void) rename(oldpath.CStr(), newpath.CStr());
	}
}

void ApiListener::LogGlobHandler(std::vector<int>& files, const String& file)
{
	String name = Utility::BaseName(file);

	if (name == "current")
		return;

	int ts;

	try {
		ts = Convert::ToLong(name);
	} catch (const std::exception&) {
		return;
	}

	files.push_back(ts);
}

void ApiListener::ReplayLog(const JsonRpcConnection::Ptr& client)
{
	Endpoint::Ptr endpoint = client->GetEndpoint();

	if (endpoint->GetLogDuration() == 0) {
		ObjectLock olock2(endpoint);
		endpoint->SetSyncing(false);
		return;
	}

	CONTEXT("Replaying log for Endpoint '" + endpoint->GetName() + "'");

	int count = -1;
	double peer_ts = endpoint->GetLocalLogPosition();
	double logpos_ts = peer_ts;
	bool last_sync = false;

	Endpoint::Ptr target_endpoint = client->GetEndpoint();
	ASSERT(target_endpoint);

	Zone::Ptr target_zone = target_endpoint->GetZone();

	if (!target_zone) {
		ObjectLock olock2(endpoint);
		endpoint->SetSyncing(false);
		return;
	}

	for (;;) {
		boost::mutex::scoped_lock lock(m_LogLock);

		CloseLogFile();

		if (count == -1 || count > 50000) {
			OpenLogFile();
			lock.unlock();
		} else {
			last_sync = true;
		}

		count = 0;

		std::vector<int> files;
		Utility::Glob(GetApiDir() + "log/*", std::bind(&ApiListener::LogGlobHandler, std::ref(files), _1), GlobFile);
		std::sort(files.begin(), files.end());

		std::vector<std::pair<int, String>> allFiles;

		for (int ts : files) {
			if (ts >= peer_ts) {
				allFiles.emplace_back(ts, GetApiDir() + "log/" + Convert::ToString(ts));
			}
		}

		allFiles.emplace_back(Utility::GetTime() + 1, GetApiDir() + "log/current");

		for (auto& file : allFiles) {
			Log(LogNotice, "ApiListener")
				<< "Replaying log: " << file.second;

			auto *fp = new std::fstream(file.second.CStr(), std::fstream::in | std::fstream::binary);
			StdioStream::Ptr logStream = new StdioStream(fp, true);

			String message;
			StreamReadContext src;
			while (true) {
				Dictionary::Ptr pmessage;

				try {
					StreamReadStatus srs = NetString::ReadStringFromStream(logStream, &message, src);

					if (srs == StatusEof)
						break;

					if (srs != StatusNewItem)
						continue;

					pmessage = JsonDecode(message);
				} catch (const std::exception&) {
					Log(LogWarning, "ApiListener")
						<< "Unexpected end-of-file for cluster log: " << file.second;

					/* Log files may be incomplete or corrupted. This is perfectly OK. */
					break;
				}

				if (pmessage->Get("timestamp") <= peer_ts)
					continue;

				Dictionary::Ptr secname = pmessage->Get("secobj");

				if (secname) {
					ConfigObject::Ptr secobj = ConfigObject::GetObject(secname->Get("type"), secname->Get("name"));

					if (!secobj)
						continue;

					if (!target_zone->CanAccessObject(secobj))
						continue;
				}

				try  {
					client->SendRawMessage(pmessage->Get("message"));
					count++;
				} catch (const std::exception& ex) {
					Log(LogWarning, "ApiListener")
						<< "Error while replaying log for endpoint '" << endpoint->GetName() << "': " << DiagnosticInformation(ex, false);

					Log(LogDebug, "ApiListener")
						<< "Error while replaying log for endpoint '" << endpoint->GetName() << "': " << DiagnosticInformation(ex);

					break;
				}

				peer_ts = pmessage->Get("timestamp");

				if (file.first > logpos_ts + 10) {
					logpos_ts = file.first;

					Dictionary::Ptr lmessage = new Dictionary({
						{ "jsonrpc", "2.0" },
						{ "method", "log::SetLogPosition" },
						{ "params", new Dictionary({
							{ "log_position", logpos_ts }
						}) }
					});

					client->SendMessage(lmessage);
				}
			}

			logStream->Close();
		}

		if (count > 0) {
			Log(LogInformation, "ApiListener")
				<< "Replayed " << count << " messages.";
		}
		else {
			Log(LogNotice, "ApiListener")
				<< "Replayed " << count << " messages.";
		}

		if (last_sync) {
			{
				ObjectLock olock2(endpoint);
				endpoint->SetSyncing(false);
			}

			OpenLogFile();

			break;
		}
	}
}

void ApiListener::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	std::pair<Dictionary::Ptr, Dictionary::Ptr> stats;

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return;

	stats = listener->GetStatus();

	ObjectLock olock(stats.second);
	for (const Dictionary::Pair& kv : stats.second)
		perfdata->Add(new PerfdataValue("api_" + kv.first, kv.second));

	status->Set("api", stats.first);
}

std::pair<Dictionary::Ptr, Dictionary::Ptr> ApiListener::GetStatus()
{
	Dictionary::Ptr perfdata = new Dictionary();

	/* cluster stats */

	double allEndpoints = 0;
	Array::Ptr allNotConnectedEndpoints = new Array();
	Array::Ptr allConnectedEndpoints = new Array();

	Zone::Ptr my_zone = Zone::GetLocalZone();

	Dictionary::Ptr connectedZones = new Dictionary();

	for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
		/* only check endpoints in a) the same zone b) our parent zone c) immediate child zones */
		if (my_zone != zone && my_zone != zone->GetParent() && zone != my_zone->GetParent()) {
			Log(LogDebug, "ApiListener")
				<< "Not checking connection to Zone '" << zone->GetName() << "' because it's not in the same zone, a parent or a child zone.";
			continue;
		}

		bool zoneConnected = false;
		int countZoneEndpoints = 0;
		double zoneLag = 0;

		ArrayData zoneEndpoints;

		for (const Endpoint::Ptr& endpoint : zone->GetEndpoints()) {
			zoneEndpoints.emplace_back(endpoint->GetName());

			if (endpoint->GetName() == GetIdentity())
				continue;

			double eplag = CalculateZoneLag(endpoint);

			if (eplag > 0 && eplag > zoneLag)
				zoneLag = eplag;

			allEndpoints++;
			countZoneEndpoints++;

			if (!endpoint->GetConnected()) {
				allNotConnectedEndpoints->Add(endpoint->GetName());
			} else {
				allConnectedEndpoints->Add(endpoint->GetName());
				zoneConnected = true;
			}
		}

		/* if there's only one endpoint inside the zone, we're not connected - that's us, fake it */
		if (zone->GetEndpoints().size() == 1 && countZoneEndpoints == 0)
			zoneConnected = true;

		String parentZoneName;
		Zone::Ptr parentZone = zone->GetParent();
		if (parentZone)
			parentZoneName = parentZone->GetName();

		Dictionary::Ptr zoneStats = new Dictionary({
			{ "connected", zoneConnected },
			{ "client_log_lag", zoneLag },
			{ "endpoints", new Array(std::move(zoneEndpoints)) },
			{ "parent_zone", parentZoneName }
		});

		connectedZones->Set(zone->GetName(), zoneStats);
	}

	/* connection stats */
	size_t jsonRpcAnonymousClients = GetAnonymousClients().size();
	size_t httpClients = GetHttpClients().size();
	size_t syncQueueItems = m_SyncQueue.GetLength();
	size_t relayQueueItems = m_RelayQueue.GetLength();
	double workQueueItemRate = JsonRpcConnection::GetWorkQueueRate();
	double syncQueueItemRate = m_SyncQueue.GetTaskCount(60) / 60.0;
	double relayQueueItemRate = m_RelayQueue.GetTaskCount(60) / 60.0;

	Dictionary::Ptr status = new Dictionary({
		{ "identity", GetIdentity() },
		{ "num_endpoints", allEndpoints },
		{ "num_conn_endpoints", allConnectedEndpoints->GetLength() },
		{ "num_not_conn_endpoints", allNotConnectedEndpoints->GetLength() },
		{ "conn_endpoints", allConnectedEndpoints },
		{ "not_conn_endpoints", allNotConnectedEndpoints },

		{ "zones", connectedZones },

		{ "json_rpc", new Dictionary({
			{ "anonymous_clients", jsonRpcAnonymousClients },
			{ "sync_queue_items", syncQueueItems },
			{ "relay_queue_items", relayQueueItems },
			{ "work_queue_item_rate", workQueueItemRate },
			{ "sync_queue_item_rate", syncQueueItemRate },
			{ "relay_queue_item_rate", relayQueueItemRate }
		}) },

		{ "http", new Dictionary({
			{ "clients", httpClients }
		}) }
	});

	/* performance data */
	perfdata->Set("num_endpoints", allEndpoints);
	perfdata->Set("num_conn_endpoints", Convert::ToDouble(allConnectedEndpoints->GetLength()));
	perfdata->Set("num_not_conn_endpoints", Convert::ToDouble(allNotConnectedEndpoints->GetLength()));

	perfdata->Set("num_json_rpc_anonymous_clients", jsonRpcAnonymousClients);
	perfdata->Set("num_http_clients", httpClients);
	perfdata->Set("num_json_rpc_sync_queue_items", syncQueueItems);
	perfdata->Set("num_json_rpc_relay_queue_items", relayQueueItems);

	perfdata->Set("num_json_rpc_work_queue_item_rate", workQueueItemRate);
	perfdata->Set("num_json_rpc_sync_queue_item_rate", syncQueueItemRate);
	perfdata->Set("num_json_rpc_relay_queue_item_rate", relayQueueItemRate);

	return std::make_pair(status, perfdata);
}

double ApiListener::CalculateZoneLag(const Endpoint::Ptr& endpoint)
{
	double remoteLogPosition = endpoint->GetRemoteLogPosition();
	double eplag = Utility::GetTime() - remoteLogPosition;

	if ((endpoint->GetSyncing() || !endpoint->GetConnected()) && remoteLogPosition != 0)
		return eplag;

	return 0;
}

bool ApiListener::AddAnonymousClient(const JsonRpcConnection::Ptr& aclient)
{
	boost::mutex::scoped_lock lock(m_AnonymousClientsLock);

	if (GetMaxAnonymousClients() >= 0 && (long)m_AnonymousClients.size() + 1 > (long)GetMaxAnonymousClients())
		return false;

	m_AnonymousClients.insert(aclient);
	return true;
}

void ApiListener::RemoveAnonymousClient(const JsonRpcConnection::Ptr& aclient)
{
	boost::mutex::scoped_lock lock(m_AnonymousClientsLock);
	m_AnonymousClients.erase(aclient);
}

std::set<JsonRpcConnection::Ptr> ApiListener::GetAnonymousClients() const
{
	boost::mutex::scoped_lock lock(m_AnonymousClientsLock);
	return m_AnonymousClients;
}

void ApiListener::AddHttpClient(const HttpServerConnection::Ptr& aclient)
{
	boost::mutex::scoped_lock lock(m_HttpClientsLock);
	m_HttpClients.insert(aclient);
}

void ApiListener::RemoveHttpClient(const HttpServerConnection::Ptr& aclient)
{
	boost::mutex::scoped_lock lock(m_HttpClientsLock);
	m_HttpClients.erase(aclient);
}

std::set<HttpServerConnection::Ptr> ApiListener::GetHttpClients() const
{
	boost::mutex::scoped_lock lock(m_HttpClientsLock);
	return m_HttpClients;
}

Value ApiListener::HelloAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	return Empty;
}

Endpoint::Ptr ApiListener::GetLocalEndpoint() const
{
	return m_LocalEndpoint;
}

void ApiListener::UpdateActivePackageStagesCache()
{
	boost::mutex::scoped_lock lock(m_ActivePackageStagesLock);

	for (auto package : ConfigPackageUtility::GetPackages()) {
		String activeStage;

		try {
			activeStage = ConfigPackageUtility::GetActiveStageFromFile(package);
		} catch (const std::exception& ex) {
			Log(LogCritical, "ApiListener")
				<< ex.what();
			continue;
		}

		Log(LogNotice, "ApiListener")
			<< "Updating cache: Config package '" << package << "' has active stage '" << activeStage << "'.";

		m_ActivePackageStages[package] = activeStage;
	}
}

void ApiListener::CheckApiPackageIntegrity()
{
	boost::mutex::scoped_lock lock(m_ActivePackageStagesLock);

	for (auto package : ConfigPackageUtility::GetPackages()) {
		String activeStage;
		try {
			activeStage = ConfigPackageUtility::GetActiveStageFromFile(package);
		} catch (const std::exception& ex) {
			/* An error means that the stage is broken, try to repair it. */
			auto it = m_ActivePackageStages.find(package);

			if (it == m_ActivePackageStages.end())
				continue;

			String activeStageCached = it->second;

			Log(LogInformation, "ApiListener")
				<< "Repairing broken API config package '" << package
				<< "', setting active stage '" << activeStageCached << "'.";

			ConfigPackageUtility::SetActiveStageToFile(package, activeStageCached);
		}
	}
}

void ApiListener::SetActivePackageStage(const String& package, const String& stage)
{
	boost::mutex::scoped_lock lock(m_ActivePackageStagesLock);
	m_ActivePackageStages[package] = stage;
}

String ApiListener::GetActivePackageStage(const String& package)
{
	boost::mutex::scoped_lock lock(m_ActivePackageStagesLock);

	if (m_ActivePackageStages.find(package) == m_ActivePackageStages.end())
		BOOST_THROW_EXCEPTION(ScriptError("Package " + package + " has no active stage."));

	return m_ActivePackageStages[package];
}

void ApiListener::RemoveActivePackageStage(const String& package)
{
	/* This is the rare occassion when a package has been deleted. */
	boost::mutex::scoped_lock lock(m_ActivePackageStagesLock);

	auto it = m_ActivePackageStages.find(package);

	if (it == m_ActivePackageStages.end())
		return;

	m_ActivePackageStages.erase(it);
}

void ApiListener::ValidateTlsProtocolmin(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ApiListener>::ValidateTlsProtocolmin(lvalue, utils);

	if (lvalue() != SSL_TXT_TLSV1
#ifdef SSL_TXT_TLSV1_1
		&& lvalue() != SSL_TXT_TLSV1_1 &&
		lvalue() != SSL_TXT_TLSV1_2
#endif /* SSL_TXT_TLSV1_1 */
		) {
		String message = "Invalid TLS version. Must be one of '" SSL_TXT_TLSV1 "'";
#ifdef SSL_TXT_TLSV1_1
		message += ", '" SSL_TXT_TLSV1_1 "' or '" SSL_TXT_TLSV1_2 "'";
#endif /* SSL_TXT_TLSV1_1 */

		BOOST_THROW_EXCEPTION(ValidationError(this, { "tls_protocolmin" }, message));
	}
}

void ApiListener::ValidateTlsHandshakeTimeout(const Lazy<double>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ApiListener>::ValidateTlsHandshakeTimeout(lvalue, utils);

	if (lvalue() <= 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "tls_handshake_timeout" }, "Value must be greater than 0."));
}

bool ApiListener::IsHACluster()
{
	Zone::Ptr zone = Zone::GetLocalZone();

	if (!zone)
		return false;

	return zone->IsSingleInstance();
}

/* Provide a helper function for zone origin name. */
String ApiListener::GetFromZoneName(const Zone::Ptr& fromZone)
{
	String fromZoneName;

	if (fromZone) {
		fromZoneName = fromZone->GetName();
	} else {
		Zone::Ptr lzone = Zone::GetLocalZone();

		if (lzone)
			fromZoneName = lzone->GetName();
	}

	return fromZoneName;
}

void ApiListener::UpdateStatusFile(boost::asio::ip::tcp::endpoint localEndpoint)
{
	String path = Configuration::CacheDir + "/api-state.json";

	Utility::SaveJsonFile(path, 0644, new Dictionary({
		{"host", String(localEndpoint.address().to_string())},
		{"port", localEndpoint.port()}
	}));
}

void ApiListener::RemoveStatusFile()
{
	String path = Configuration::CacheDir + "/api-state.json";

	Utility::Remove(path);
}
