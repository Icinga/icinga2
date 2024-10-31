/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "remote/apilistener.hpp"
#include "remote/apilistener-ti.cpp"
#include "remote/jsonrpcconnection.hpp"
#include "remote/endpoint.hpp"
#include "remote/jsonrpc.hpp"
#include "remote/apifunction.hpp"
#include "remote/configpackageutility.hpp"
#include "remote/configobjectutility.hpp"
#include "base/atomic-file.hpp"
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
#include <boost/asio/io_context_strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/context.hpp>
#include <boost/date_time/posix_time/posix_time_duration.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/regex.hpp>
#include <boost/system/error_code.hpp>
#include <boost/thread/locks.hpp>
#include <climits>
#include <cstdint>
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

String ApiListener::GetApiZonesDir()
{
	return GetApiDir() + "zones/";
}

String ApiListener::GetApiZonesStageDir()
{
	return GetApiDir() + "zones-stage/";
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

std::shared_ptr<X509> ApiListener::RenewCert(const std::shared_ptr<X509>& cert, bool ca)
{
	std::shared_ptr<EVP_PKEY> pubkey (X509_get_pubkey(cert.get()), EVP_PKEY_free);
	auto subject (X509_get_subject_name(cert.get()));
	auto cacert (GetX509Certificate(GetDefaultCaPath()));
	auto newcert (CreateCertIcingaCA(pubkey.get(), subject, ca));

	/* verify that the new cert matches the CA we're using for the ApiListener;
	 * this ensures that the CA we have in /var/lib/icinga2/ca matches the one
	 * we're using for cluster connections (there's no point in sending a client
	 * a certificate it wouldn't be able to use to connect to us anyway) */
	try {
		if (!VerifyCertificate(cacert, newcert, GetCrlPath())) {
			Log(LogWarning, "ApiListener")
				<< "The CA in '" << GetDefaultCaPath() << "' does not match the CA which Icinga uses "
				<< "for its own cluster connections. This is most likely a configuration problem.";

			return nullptr;
		}
	} catch (const std::exception&) { } /* Swallow the exception on purpose, cacert will never be a non-CA certificate. */

	return newcert;
}

void ApiListener::UpdateSSLContext()
{
	auto ctx (SetupSslContext(GetDefaultCertPath(), GetDefaultKeyPath(), GetDefaultCaPath(), GetCrlPath(), GetCipherList(), GetTlsProtocolmin(), GetDebugInfo()));

	{
		boost::unique_lock<decltype(m_SSLContextMutex)> lock (m_SSLContextMutex);

		m_SSLContext = std::move(ctx);
	}

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

	SyncLocalZoneDirs();

	m_RenewOwnCertTimer = Timer::Create();

	if (Utility::PathExists(GetIcingaCADir() + "/ca.key")) {
		RenewOwnCert();
		RenewCA();

		m_RenewOwnCertTimer->OnTimerExpired.connect([this](const Timer * const&) {
			RenewOwnCert();
			RenewCA();
		});
	} else {
		m_RenewOwnCertTimer->OnTimerExpired.connect([this](const Timer * const&) {
			JsonRpcConnection::SendCertificateRequest(nullptr, nullptr, String());
		});
	}

	m_RenewOwnCertTimer->SetInterval(RENEW_INTERVAL);
	m_RenewOwnCertTimer->Start();

	ObjectImpl<ApiListener>::Start(runtimeCreated);

	{
		std::unique_lock<std::mutex> lock(m_LogLock);
		OpenLogFile();
	}

	/* create the primary JSON-RPC listener */
	if (!AddListener(GetBindHost(), GetBindPort())) {
		Log(LogCritical, "ApiListener")
			<< "Cannot add listener on host '" << GetBindHost() << "' for port '" << GetBindPort() << "'.";
		Application::Exit(EXIT_FAILURE);
	}

	m_Timer = Timer::Create();
	m_Timer->OnTimerExpired.connect([this](const Timer * const&) { ApiTimerHandler(); });
	m_Timer->SetInterval(5);
	m_Timer->Start();
	m_Timer->Reschedule(0);

	m_ReconnectTimer = Timer::Create();
	m_ReconnectTimer->OnTimerExpired.connect([this](const Timer * const&) { ApiReconnectTimerHandler(); });
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	/* Keep this in relative sync with the cold startup in UpdateObjectAuthority() and the reconnect interval above.
	 * Previous: 60s reconnect, 30s OA, 60s cold startup.
	 * Now: 10s reconnect, 10s OA, 30s cold startup.
	 */
	m_AuthorityTimer = Timer::Create();
	m_AuthorityTimer->OnTimerExpired.connect([](const Timer * const&) { UpdateObjectAuthority(); });
	m_AuthorityTimer->SetInterval(10);
	m_AuthorityTimer->Start();

	m_CleanupCertificateRequestsTimer = Timer::Create();
	m_CleanupCertificateRequestsTimer->OnTimerExpired.connect([this](const Timer * const&) { CleanupCertificateRequestsTimerHandler(); });
	m_CleanupCertificateRequestsTimer->SetInterval(3600);
	m_CleanupCertificateRequestsTimer->Start();
	m_CleanupCertificateRequestsTimer->Reschedule(0);

	m_ApiPackageIntegrityTimer = Timer::Create();
	m_ApiPackageIntegrityTimer->OnTimerExpired.connect([this](const Timer * const&) { CheckApiPackageIntegrity(); });
	m_ApiPackageIntegrityTimer->SetInterval(300);
	m_ApiPackageIntegrityTimer->Start();

	OnMasterChanged(true);
}

void ApiListener::RenewOwnCert()
{
	auto certPath (GetDefaultCertPath());
	auto cert (GetX509Certificate(certPath));

	if (IsCertUptodate(cert)) {
		return;
	}

	Log(LogInformation, "ApiListener")
		<< "Our certificate will expire soon, but we own the CA. Renewing.";

	cert = RenewCert(cert);

	if (!cert) {
		return;
	}

	AtomicFile::Write(certPath, 0644, CertificateToString(cert));
	UpdateSSLContext();
}

void ApiListener::RenewCA()
{
	auto certPath (GetCaDir() + "/ca.crt");
	auto cert (GetX509Certificate(certPath));

	if (IsCaUptodate(cert.get())) {
		return;
	}

	Log(LogInformation, "ApiListener")
		<< "Our CA will expire soon, but we own it. Renewing.";

	cert = RenewCert(cert, true);

	if (!cert) {
		return;
	}

	auto certStr (CertificateToString(cert));

	AtomicFile::Write(GetDefaultCaPath(), 0644, certStr);
	AtomicFile::Write(certPath, 0644, certStr);
	UpdateSSLContext();
}

void ApiListener::Stop(bool runtimeDeleted)
{
	m_ApiPackageIntegrityTimer->Stop(true);
	m_CleanupCertificateRequestsTimer->Stop(true);
	m_AuthorityTimer->Stop(true);
	m_ReconnectTimer->Stop(true);
	m_Timer->Stop(true);
	m_RenewOwnCertTimer->Stop(true);

	ObjectImpl<ApiListener>::Stop(runtimeDeleted);

	Log(LogInformation, "ApiListener")
		<< "'" << GetName() << "' stopped.";

	{
		std::unique_lock<std::mutex> lock(m_LogLock);
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

	if (!m_SSLContext) {
		Log(LogCritical, "ApiListener", "SSL context is required for AddListener()");
		return false;
	}

	auto& io (IoEngine::Get().GetIoContext());
	auto acceptor (Shared<tcp::acceptor>::Make(io));

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
#ifdef SO_REUSEPORT
					setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, reinterpret_cast<const char *>(&optTrue), sizeof(optTrue));
#endif /* SO_REUSEPORT */
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
			<< "Cannot bind TCP socket for host '" << node << "' on port '" << service << "': " << ex.what();
		return false;
	}

	acceptor->listen(INT_MAX);

	auto localEndpoint (acceptor->local_endpoint());

	Log(LogInformation, "ApiListener")
		<< "Started new listener on '[" << localEndpoint.address() << "]:" << localEndpoint.port() << "'";

	IoEngine::SpawnCoroutine(io, [this, acceptor](asio::yield_context yc) { ListenerCoroutineProc(yc, acceptor); });

	UpdateStatusFile(localEndpoint);

	return true;
}

void ApiListener::ListenerCoroutineProc(boost::asio::yield_context yc, const Shared<boost::asio::ip::tcp::acceptor>::Ptr& server)
{
	namespace asio = boost::asio;

	auto& io (IoEngine::Get().GetIoContext());

	time_t lastModified = -1;
	const String crlPath = GetCrlPath();

	if (!crlPath.IsEmpty()) {
		lastModified = Utility::GetFileCreationTime(crlPath);
	}

	for (;;) {
		try {
			asio::ip::tcp::socket socket (io);

			server->async_accept(socket.lowest_layer(), yc);

			auto remoteEndpoint (socket.lowest_layer().remote_endpoint());

			if (!crlPath.IsEmpty()) {
				time_t currentCreationTime = Utility::GetFileCreationTime(crlPath);

				if (lastModified != currentCreationTime) {
					UpdateSSLContext();

					lastModified = currentCreationTime;
				}
			}

			boost::shared_lock<decltype(m_SSLContextMutex)> lock (m_SSLContextMutex);
			auto sslConn (Shared<AsioTlsStream>::Make(io, *m_SSLContext));

			lock.unlock();
			sslConn->lowest_layer() = std::move(socket);

			auto strand (Shared<asio::io_context::strand>::Make(io));

			IoEngine::SpawnCoroutine(*strand, [this, strand, sslConn, remoteEndpoint](asio::yield_context yc) {
				Timeout::Ptr timeout(new Timeout(strand->context(), *strand, boost::posix_time::microseconds(int64_t(GetConnectTimeout() * 1e6)),
					[sslConn, remoteEndpoint](asio::yield_context yc) {
						Log(LogWarning, "ApiListener")
							<< "Timeout while processing incoming connection from " << remoteEndpoint;

						boost::system::error_code ec;
						sslConn->lowest_layer().cancel(ec);
					}
				));
				Defer cancelTimeout([timeout]() { timeout->Cancel(); });

				NewClientHandler(yc, strand, sslConn, String(), RoleServer);
			});
		} catch (const std::exception& ex) {
			Log(LogCritical, "ApiListener")
				<< "Cannot accept new connection: " << ex.what();
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

	if (!m_SSLContext) {
		Log(LogCritical, "ApiListener", "SSL context is required for AddConnection()");
		return;
	}

	auto& io (IoEngine::Get().GetIoContext());
	auto strand (Shared<asio::io_context::strand>::Make(io));

	IoEngine::SpawnCoroutine(*strand, [this, strand, endpoint, &io](asio::yield_context yc) {
		String host = endpoint->GetHost();
		String port = endpoint->GetPort();

		Log(LogInformation, "ApiListener")
			<< "Reconnecting to endpoint '" << endpoint->GetName() << "' via host '" << host << "' and port '" << port << "'";

		try {
			boost::shared_lock<decltype(m_SSLContextMutex)> lock (m_SSLContextMutex);
			auto sslConn (Shared<AsioTlsStream>::Make(io, *m_SSLContext, endpoint->GetName()));

			lock.unlock();

			Timeout::Ptr timeout(new Timeout(strand->context(), *strand, boost::posix_time::microseconds(int64_t(GetConnectTimeout() * 1e6)),
				[sslConn, endpoint, host, port](asio::yield_context yc) {
					Log(LogCritical, "ApiListener")
						<< "Timeout while reconnecting to endpoint '" << endpoint->GetName() << "' via host '" << host
						<< "' and port '" << port << "', cancelling attempt";

					boost::system::error_code ec;
					sslConn->lowest_layer().cancel(ec);
				}
			));
			Defer cancelTimeout([&timeout]() { timeout->Cancel(); });

			Connect(sslConn->lowest_layer(), host, port, yc);

			NewClientHandler(yc, strand, sslConn, endpoint->GetName(), RoleClient);

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

void ApiListener::NewClientHandler(
	boost::asio::yield_context yc, const Shared<boost::asio::io_context::strand>::Ptr& strand,
	const Shared<AsioTlsStream>::Ptr& client, const String& hostname, ConnectionRole role
)
{
	try {
		NewClientHandlerInternal(yc, strand, client, hostname, role);
	} catch (const std::exception& ex) {
		Log(LogCritical, "ApiListener")
			<< "Exception while handling new API client connection: " << DiagnosticInformation(ex, false);

		Log(LogDebug, "ApiListener")
			<< "Exception while handling new API client connection: " << DiagnosticInformation(ex);
	}
}

static const auto l_AppVersionInt (([]() -> unsigned long {
	auto appVersion (Application::GetAppVersion());
	boost::regex rgx (R"EOF(^[rv]?(\d+)\.(\d+)\.(\d+))EOF");
	boost::smatch match;

	if (!boost::regex_search(appVersion.GetData(), match, rgx)) {
		return 0;
	}

	return 100u * 100u * boost::lexical_cast<unsigned long>(match[1].str())
		+ 100u * boost::lexical_cast<unsigned long>(match[2].str())
		+ boost::lexical_cast<unsigned long>(match[3].str());
})());

static const auto l_MyCapabilities (
	(uint_fast64_t)ApiCapabilities::ExecuteArbitraryCommand | (uint_fast64_t)ApiCapabilities::IfwApiCheckCommand
);

/**
 * Processes a new client connection.
 *
 * @param client The new client.
 */
void ApiListener::NewClientHandlerInternal(
	boost::asio::yield_context yc, const Shared<boost::asio::io_context::strand>::Ptr& strand,
	const Shared<AsioTlsStream>::Ptr& client, const String& hostname, ConnectionRole role
)
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

	boost::system::error_code ec;

	{
		Timeout::Ptr handshakeTimeout (new Timeout(
			strand->context(),
			*strand,
			boost::posix_time::microseconds(intmax_t(Configuration::TlsHandshakeTimeout * 1000000)),
			[strand, client](asio::yield_context yc) {
				boost::system::error_code ec;
				client->lowest_layer().cancel(ec);
			}
		));

		sslConn.async_handshake(role == RoleClient ? sslConn.client : sslConn.server, yc[ec]);

		handshakeTimeout->Cancel();
	}

	if (ec) {
		// https://github.com/boostorg/beast/issues/915
		// Google Chrome 73+ seems not close the connection properly, https://stackoverflow.com/questions/56272906/how-to-fix-certificate-unknown-error-from-chrome-v73
		if (ec == asio::ssl::error::stream_truncated) {
			Log(LogNotice, "ApiListener")
				<< "TLS stream was truncated, ignoring connection from " << conninfo;
			return;
		}

		Log(LogCritical, "ApiListener")
			<< "Client TLS handshake failed (" << conninfo << "): " << ec.message();
		return;
	}

	bool willBeShutDown = false;

	Defer shutDownIfNeeded ([&sslConn, &willBeShutDown, &yc]() {
		if (!willBeShutDown) {
			// Ignore the error, but do not throw an exception being swallowed at all cost.
			// https://github.com/Icinga/icinga2/issues/7351
			boost::system::error_code ec;
			sslConn.async_shutdown(yc[ec]);
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
				<< "Cannot get certificate common name from peer (" << conninfo << ") cert.";
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

	try {
		if (role == RoleClient) {
			JsonRpc::SendMessage(client, new Dictionary({
				{ "jsonrpc", "2.0" },
				{ "method", "icinga::Hello" },
				{ "params", new Dictionary({
					{ "version", (double)l_AppVersionInt },
					{ "capabilities", (double)l_MyCapabilities }
				}) }
			}), yc);

			client->async_flush(yc);

			ctype = ClientJsonRpc;
		} else {
			{
				boost::system::error_code ec;

				if (client->async_fill(yc[ec]) == 0u) {
					if (identity.IsEmpty()) {
						Log(LogInformation, "ApiListener")
							<< "No data received on new API connection " << conninfo << ". "
							<< "Ensure that the remote endpoints are properly configured in a cluster setup.";
					} else {
						Log(LogWarning, "ApiListener")
							<< "No data received on new API connection " << conninfo << " for identity '" << identity << "'. "
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
				JsonRpc::SendMessage(client, new Dictionary({
					{ "jsonrpc", "2.0" },
					{ "method", "icinga::Hello" },
					{ "params", new Dictionary({
						{ "version", (double)l_AppVersionInt },
						{ "capabilities", (double)l_MyCapabilities }
					}) }
				}), yc);

				client->async_flush(yc);

				ctype = ClientJsonRpc;
			} else {
				ctype = ClientHttp;
			}
		}
	} catch (const boost::system::system_error& systemError) {
		if (systemError.code() == boost::asio::error::operation_aborted) {
			shutDownIfNeeded.Cancel();
		}

		throw;
	}

	if (ctype == ClientJsonRpc) {
		Log(LogNotice, "ApiListener", "New JSON-RPC client");

		if (endpoint && endpoint->GetConnected()) {
			Log(LogInformation, "ApiListener")
				<< "Ignoring JSON-RPC connection " << conninfo
				<< ". We're already connected to Endpoint '" << endpoint->GetName()
				<< "' (last message sent: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", endpoint->GetLastMessageSent())
				<< ", last message received: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S", endpoint->GetLastMessageReceived()) << ").";
			return;
		}

		JsonRpcConnection::Ptr aclient = new JsonRpcConnection(identity, verify_ok, client, role);

		if (endpoint) {
			endpoint->AddClient(aclient);

			Utility::QueueAsyncCallback([this, aclient, endpoint]() {
				SyncClient(aclient, endpoint, true);
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
		auto parent (myZone->GetParent());

		if (parent == eZone || (!parent && eZone == myZone)) {
			JsonRpcConnection::SendCertificateRequest(aclient, nullptr, String());

			if (Utility::PathExists(ApiListener::GetCertificateRequestsDir())) {
				Utility::Glob(ApiListener::GetCertificateRequestsDir() + "/*.json", [aclient](const String& newPath) {
					JsonRpcConnection::SendCertificateRequest(aclient, nullptr, newPath);
				}, GlobFile);
			}
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
	Utility::Glob(GetApiDir() + "log/*", [&files](const String& file) { LogGlobHandler(files, file); }, GlobFile);
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

		Log(LogNotice, "ApiListener")
			<< "Setting log position for identity '" << endpoint->GetName() << "': "
			<< Utility::FormatDateTime("%Y/%m/%d %H:%M:%S", ts);

		for (const JsonRpcConnection::Ptr& client : endpoint->GetClients()) {
			if (client->GetTimestamp() == maxTs) {
				try {
					client->SendMessage(lmessage);
				} catch (const std::runtime_error& ex) {
					Log(LogNotice, "ApiListener")
						<< "Error while setting log position for identity '" << endpoint->GetName() << "': " << DiagnosticInformation(ex, false);
				}
			} else {
				client->Disconnect();
			}
		}
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
		Utility::Glob(requestsDir + "/*.json", [expiryTime](const String& path) {
			CleanupCertificateRequest(path, expiryTime);
		}, GlobFile);
	}
}

void ApiListener::RelayMessage(const MessageOrigin::Ptr& origin,
	const ConfigObject::Ptr& secobj, const Dictionary::Ptr& message, bool log)
{
	if (!IsActive())
		return;

	m_RelayQueue.Enqueue([this, origin, secobj, message, log]() { SyncRelayMessage(origin, secobj, message, log); }, PriorityNormal, true);
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

	std::unique_lock<std::mutex> lock(m_LogLock);
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

			try {
				client->SendMessage(message);
			} catch (const std::runtime_error& ex) {
				Log(LogNotice, "ApiListener")
					<< "Error while sending message to endpoint '" << endpoint->GetName() << "': " << DiagnosticInformation(ex, false);
			}
		}
	}
}

/**
 * Relay a message to a directly connected zone or to a global zone.
 * If some other zone is passed as the target zone, it is not relayed.
 *
 * @param targetZone The zone to relay to
 * @param origin Information about where this message is relayed from (if it was not generated locally)
 * @param message The message to relay
 * @param currentZoneMaster The current master node of the local zone
 * @return true if the message has been relayed to all relevant endpoints,
 *         false if it hasn't and must be persisted in the replay log
 */
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

	std::set<Zone::Ptr> allTargetZones;
	if (targetZone->GetGlobal()) {
		/* if the zone is global, the message has to be relayed to our local zone and direct children */
		allTargetZones.insert(localZone);
		for (const Zone::Ptr& zone : ConfigType::GetObjectsByType<Zone>()) {
			if (zone->GetParent() == localZone) {
				allTargetZones.insert(zone);
			}
		}
	} else {
		/* whereas if it's not global, the message is just relayed to the zone itself */
		allTargetZones.insert(targetZone);
	}

	bool needsReplay = false;

	for (const Zone::Ptr& currentTargetZone : allTargetZones) {
		bool relayed = false, log_needed = false, log_done = false;

		for (const Endpoint::Ptr& targetEndpoint : currentTargetZone->GetEndpoints()) {
			/* Don't relay messages to ourselves. */
			if (targetEndpoint == localEndpoint)
				continue;

			log_needed = true;

			/* Don't relay messages to disconnected endpoints. */
			if (!targetEndpoint->GetConnected()) {
				if (currentTargetZone == localZone)
					log_done = false;

				continue;
			}

			log_done = true;

			/* Don't relay the message to the zone through more than one endpoint unless this is our own zone.
			 * 'relayed' is set to true on success below, enabling the checks in the second iteration.
			 */
			if (relayed && currentTargetZone != localZone) {
				skippedEndpoints.push_back(targetEndpoint);
				continue;
			}

			/* Don't relay messages back to the endpoint which we got the message from. */
			if (origin && origin->FromClient && targetEndpoint == origin->FromClient->GetEndpoint()) {
				skippedEndpoints.push_back(targetEndpoint);
				continue;
			}

			/* Don't relay messages back to the zone which we got the message from. */
			if (origin && origin->FromZone && currentTargetZone == origin->FromZone) {
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

		if (log_needed && !log_done) {
			needsReplay = true;
		}
	}

	if (!skippedEndpoints.empty()) {
		double ts = message->Get("ts");

		for (const Endpoint::Ptr& skippedEndpoint : skippedEndpoints)
			skippedEndpoint->SetLocalLogPosition(ts);
	}

	return !needsReplay;
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

	std::unique_ptr<std::fstream> fp = std::make_unique<std::fstream>(path.CStr(), std::fstream::out | std::ofstream::app);

	if (!fp->good()) {
		Log(LogWarning, "ApiListener")
			<< "Could not open spool file: " << path;
		return;
	}

	m_LogFile = new StdioStream(fp.release(), true);
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
		try {
			Utility::RenameFile(oldpath, newpath);

			// We're rotating the current log file, so reset the log message counter as well.
			m_LogMessageCount = 0;
		} catch (const std::exception& ex) {
			Log(LogCritical, "ApiListener")
				<< "Cannot rotate replay log file from '" << oldpath << "' to '"
				<< newpath << "': " << ex.what();
		}
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
	Defer resetEndpointSyncing ([&endpoint]() {
		ObjectLock olock(endpoint);
		endpoint->SetSyncing(false);
	});

	if (endpoint->GetLogDuration() == 0) {
		return;
	}

	CONTEXT("Replaying log for Endpoint '" << endpoint->GetName() << "'");

	int count = -1;
	double peer_ts = endpoint->GetLocalLogPosition();
	double logpos_ts = peer_ts;
	bool last_sync = false;

	Endpoint::Ptr target_endpoint = client->GetEndpoint();
	ASSERT(target_endpoint);

	Zone::Ptr target_zone = target_endpoint->GetZone();

	if (!target_zone) {
		return;
	}

	for (;;) {
		std::unique_lock<std::mutex> lock(m_LogLock);

		CloseLogFile();
		Defer reopenLog;

		if (count == -1 || count > 50000) {
			OpenLogFile();
			lock.unlock();
		} else {
			last_sync = true;
			reopenLog.SetFunc([this]() { OpenLogFile(); });
		}

		count = 0;

		std::vector<int> files;
		Utility::Glob(GetApiDir() + "log/*", [&files](const String& file) { LogGlobHandler(files, file); }, GlobFile);
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
						<< "Error while replaying log for endpoint '" << endpoint->GetName() << "': " << ex.what();

					Log(LogDebug, "ApiListener")
						<< "Error while replaying log for endpoint '" << endpoint->GetName() << "': " << DiagnosticInformation(ex);
					return;
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
			return;
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
	std::unique_lock<std::mutex> lock(m_AnonymousClientsLock);

	if (GetMaxAnonymousClients() >= 0 && (long)m_AnonymousClients.size() + 1 > (long)GetMaxAnonymousClients())
		return false;

	m_AnonymousClients.insert(aclient);
	return true;
}

void ApiListener::RemoveAnonymousClient(const JsonRpcConnection::Ptr& aclient)
{
	std::unique_lock<std::mutex> lock(m_AnonymousClientsLock);
	m_AnonymousClients.erase(aclient);
}

std::set<JsonRpcConnection::Ptr> ApiListener::GetAnonymousClients() const
{
	std::unique_lock<std::mutex> lock(m_AnonymousClientsLock);
	return m_AnonymousClients;
}

void ApiListener::AddHttpClient(const HttpServerConnection::Ptr& aclient)
{
	std::unique_lock<std::mutex> lock(m_HttpClientsLock);
	m_HttpClients.insert(aclient);
}

void ApiListener::RemoveHttpClient(const HttpServerConnection::Ptr& aclient)
{
	std::unique_lock<std::mutex> lock(m_HttpClientsLock);
	m_HttpClients.erase(aclient);
}

std::set<HttpServerConnection::Ptr> ApiListener::GetHttpClients() const
{
	std::unique_lock<std::mutex> lock(m_HttpClientsLock);
	return m_HttpClients;
}

static void LogAppVersion(unsigned long version, Log& log)
{
	log << version / 100u << "." << version % 100u << ".x";
}

Value ApiListener::HelloAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	if (origin) {
		auto client (origin->FromClient);

		if (client) {
			auto endpoint (client->GetEndpoint());

			if (endpoint) {
				unsigned long nodeVersion = params->Get("version");

				endpoint->SetIcingaVersion(nodeVersion);
				endpoint->SetCapabilities((double)params->Get("capabilities"));

				if (nodeVersion == 0u) {
					nodeVersion = 21200;
				}

				if (endpoint->GetZone()->GetParent() == Zone::GetLocalZone()) {
					switch (l_AppVersionInt / 100 - nodeVersion / 100) {
						case 0:
						case 1:
							break;
						default:
							Log log (LogWarning, "ApiListener");
							log << "Unexpected Icinga version of endpoint '" << endpoint->GetName() << "': ";

							LogAppVersion(nodeVersion / 100u, log);
							log << " Expected one of: ";

							LogAppVersion(l_AppVersionInt / 100u, log);
							log << ", ";

							LogAppVersion((l_AppVersionInt / 100u - 1u), log);
					}
				}
			}
		}
	}

	return Empty;
}

Endpoint::Ptr ApiListener::GetLocalEndpoint() const
{
	return m_LocalEndpoint;
}

void ApiListener::UpdateActivePackageStagesCache()
{
	std::unique_lock<std::mutex> lock(m_ActivePackageStagesLock);

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
	std::unique_lock<std::mutex> lock(m_ActivePackageStagesLock);

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
	std::unique_lock<std::mutex> lock(m_ActivePackageStagesLock);
	m_ActivePackageStages[package] = stage;
}

String ApiListener::GetActivePackageStage(const String& package)
{
	std::unique_lock<std::mutex> lock(m_ActivePackageStagesLock);

	if (m_ActivePackageStages.find(package) == m_ActivePackageStages.end())
		BOOST_THROW_EXCEPTION(ScriptError("Package " + package + " has no active stage."));

	return m_ActivePackageStages[package];
}

void ApiListener::RemoveActivePackageStage(const String& package)
{
	/* This is the rare occassion when a package has been deleted. */
	std::unique_lock<std::mutex> lock(m_ActivePackageStagesLock);

	auto it = m_ActivePackageStages.find(package);

	if (it == m_ActivePackageStages.end())
		return;

	m_ActivePackageStages.erase(it);
}

void ApiListener::ValidateTlsProtocolmin(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<ApiListener>::ValidateTlsProtocolmin(lvalue, utils);

	try {
		ResolveTlsProtocolVersion(lvalue());
	} catch (const std::exception& ex) {
		BOOST_THROW_EXCEPTION(ValidationError(this, { "tls_protocolmin" }, ex.what()));
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
