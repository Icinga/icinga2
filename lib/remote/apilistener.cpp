/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "remote/apilistener.hpp"
#include "remote/apilistener.tcpp"
#include "remote/jsonrpcconnection.hpp"
#include "remote/endpoint.hpp"
#include "remote/jsonrpc.hpp"
#include "remote/apifunction.hpp"
#include "base/convert.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/objectlock.hpp"
#include "base/stdiostream.hpp"
#include "base/application.hpp"
#include "base/context.hpp"
#include "base/statsfunction.hpp"
#include "base/exception.hpp"
#include <fstream>

using namespace icinga;

REGISTER_TYPE(ApiListener);

boost::signals2::signal<void(bool)> ApiListener::OnMasterChanged;
ApiListener::Ptr ApiListener::m_Instance;

REGISTER_STATSFUNCTION(ApiListener, &ApiListener::StatsFunc);

REGISTER_APIFUNCTION(Hello, icinga, &ApiListener::HelloAPIHandler);

ApiListener::ApiListener(void)
	: m_SyncQueue(0, 4), m_LogMessageCount(0)
{ }

void ApiListener::OnConfigLoaded(void)
{
	if (m_Instance)
		BOOST_THROW_EXCEPTION(ScriptError("Only one ApiListener object is allowed.", GetDebugInfo()));

	m_Instance = this;

	/* set up SSL context */
	boost::shared_ptr<X509> cert;
	try {
		cert = GetX509Certificate(GetCertPath());
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ScriptError("Cannot get certificate from cert path: '"
		    + GetCertPath() + "'.", GetDebugInfo()));
	}

	try {
		SetIdentity(GetCertificateCN(cert));
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ScriptError("Cannot get certificate common name from cert path: '"
		    + GetCertPath() + "'.", GetDebugInfo()));
	}

	Log(LogInformation, "ApiListener")
	    << "My API identity: " << GetIdentity();

	try {
		m_SSLContext = MakeSSLContext(GetCertPath(), GetKeyPath(), GetCaPath());
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ScriptError("Cannot make SSL context for cert path: '"
		    + GetCertPath() + "' key path: '" + GetKeyPath() + "' ca path: '" + GetCaPath() + "'.", GetDebugInfo()));
	}

	if (!GetCrlPath().IsEmpty()) {
		try {
			AddCRLToSSLContext(m_SSLContext, GetCrlPath());
		} catch (const std::exception&) {
			BOOST_THROW_EXCEPTION(ScriptError("Cannot add certificate revocation list to SSL context for crl path: '"
			    + GetCrlPath() + "'.", GetDebugInfo()));
		}
	}

	if (!GetCipherList().IsEmpty()) {
	  try {
	    SetCipherListToSSLContext(m_SSLContext, GetCipherList());
	  } catch (const std::exception&) {
	    BOOST_THROW_EXCEPTION(ScriptError("Cannot set cipher list to SSL context for cipher list: '"
					      + GetCipherList() + "'.", GetDebugInfo()));
	  }
	}
}

void ApiListener::OnAllConfigLoaded(void)
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
	SyncZoneDirs();

	ObjectImpl<ApiListener>::Start(runtimeCreated);

	{
		boost::mutex::scoped_lock(m_LogLock);
		RotateLogFile();
		OpenLogFile();
	}

	/* create the primary JSON-RPC listener */
	if (!AddListener(GetBindHost(), GetBindPort())) {
		Log(LogCritical, "ApiListener")
		     << "Cannot add listener on host '" << GetBindHost() << "' for port '" << GetBindPort() << "'.";
		Application::Exit(EXIT_FAILURE);
	}

	m_Timer = new Timer();
	m_Timer->OnTimerExpired.connect(boost::bind(&ApiListener::ApiTimerHandler, this));
	m_Timer->SetInterval(5);
	m_Timer->Start();
	m_Timer->Reschedule(0);

	OnMasterChanged(true);
}

ApiListener::Ptr ApiListener::GetInstance(void)
{
	return m_Instance;
}

boost::shared_ptr<SSL_CTX> ApiListener::GetSSLContext(void) const
{
	return m_SSLContext;
}

Endpoint::Ptr ApiListener::GetMaster(void) const
{
	Zone::Ptr zone = Zone::GetLocalZone();

	if (!zone)
		return Endpoint::Ptr();

	std::vector<String> names;

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, zone->GetEndpoints())
		if (endpoint->GetConnected() || endpoint->GetName() == GetIdentity())
			names.push_back(endpoint->GetName());

	std::sort(names.begin(), names.end());

	return Endpoint::GetByName(*names.begin());
}

bool ApiListener::IsMaster(void) const
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
	ObjectLock olock(this);

	boost::shared_ptr<SSL_CTX> sslContext = m_SSLContext;

	if (!sslContext) {
		Log(LogCritical, "ApiListener", "SSL context is required for AddListener()");
		return false;
	}

	Log(LogInformation, "ApiListener")
	    << "Adding new listener on port '" << service << "'";

	TcpSocket::Ptr server = new TcpSocket();

	try {
		server->Bind(node, service, AF_UNSPEC);
	} catch (const std::exception&) {
		Log(LogCritical, "ApiListener")
		    << "Cannot bind TCP socket for host '" << node << "' on port '" << service << "'.";
		return false;
	}

	boost::thread thread(boost::bind(&ApiListener::ListenerThreadProc, this, server));
	thread.detach();

	m_Servers.insert(server);

	return true;
}

void ApiListener::ListenerThreadProc(const Socket::Ptr& server)
{
	Utility::SetThreadName("API Listener");

	server->Listen();

	for (;;) {
		try {
			Socket::Ptr client = server->Accept();
			boost::thread thread(boost::bind(&ApiListener::NewClientHandler, this, client, String(), RoleServer));
			thread.detach();
		} catch (const std::exception&) {
			Log(LogCritical, "ApiListener", "Cannot accept new connection.");
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
	{
		ObjectLock olock(this);

		boost::shared_ptr<SSL_CTX> sslContext = m_SSLContext;

		if (!sslContext) {
			Log(LogCritical, "ApiListener", "SSL context is required for AddConnection()");
			return;
		}
	}

	String host = endpoint->GetHost();
	String port = endpoint->GetPort();

	Log(LogInformation, "JsonRpcConnection")
	    << "Reconnecting to API endpoint '" << endpoint->GetName() << "' via host '" << host << "' and port '" << port << "'";

	TcpSocket::Ptr client = new TcpSocket();

	try {
		endpoint->SetConnecting(true);
		client->Connect(host, port);
		NewClientHandler(client, endpoint->GetName(), RoleClient);
		endpoint->SetConnecting(false);
	} catch (const std::exception& ex) {
		endpoint->SetConnecting(false);
		client->Close();

		std::ostringstream info;
		info << "Cannot connect to host '" << host << "' on port '" << port << "'";
		Log(LogCritical, "ApiListener", info.str());
		Log(LogDebug, "ApiListener")
		    << info.str() << "\n" << DiagnosticInformation(ex);
	}
}

void ApiListener::NewClientHandler(const Socket::Ptr& client, const String& hostname, ConnectionRole role)
{
	try {
		NewClientHandlerInternal(client, hostname, role);
	} catch (const std::exception& ex) {
		Log(LogCritical, "ApiListener")
		    << "Exception while handling new API client connection: " << DiagnosticInformation(ex);
	}
}

/**
 * Processes a new client connection.
 *
 * @param client The new client.
 */
void ApiListener::NewClientHandlerInternal(const Socket::Ptr& client, const String& hostname, ConnectionRole role)
{
	CONTEXT("Handling new API client connection");

	TlsStream::Ptr tlsStream;

	{
		ObjectLock olock(this);
		try {
			tlsStream = new TlsStream(client, hostname, role, m_SSLContext);
		} catch (const std::exception&) {
			Log(LogCritical, "ApiListener", "Cannot create TLS stream from client connection.");
			return;
		}
	}

	try {
		tlsStream->Handshake();
	} catch (const std::exception& ex) {
		Log(LogCritical, "ApiListener", "Client TLS handshake failed");
		return;
	}

	boost::shared_ptr<X509> cert = tlsStream->GetPeerCertificate();
	String identity;
	Endpoint::Ptr endpoint;
	bool verify_ok = false;

	if (cert) {
		try {
			identity = GetCertificateCN(cert);
		} catch (const std::exception&) {
			Log(LogCritical, "ApiListener")
			    << "Cannot get certificate common name from cert path: '" << GetCertPath() << "'.";
			return;
		}

		verify_ok = tlsStream->IsVerifyOK();
		if (!hostname.IsEmpty()) {
			if (identity != hostname) {
				Log(LogWarning, "ApiListener")
					<< "Unexpected certificate common name while connecting to endpoint '"
				    << hostname << "': got '" << identity << "'";
				return;
			} else if (!verify_ok) {
				Log(LogWarning, "ApiListener")
					<< "Peer certificate for endpoint '" << hostname
					<< "' is not signed by the certificate authority.";
				return;
			}
		}

		Log(LogInformation, "ApiListener")
		    << "New client connection for identity '" << identity << "'"
		    << (verify_ok ? "" : " (client certificate not signed by CA)");

		if (verify_ok)
			endpoint = Endpoint::GetByName(identity);
	} else {
		Log(LogInformation, "ApiListener")
		    << "New client connection (no client certificate)";
	}

	ClientType ctype;

	if (role == RoleClient) {
		Dictionary::Ptr message = new Dictionary();
		message->Set("jsonrpc", "2.0");
		message->Set("method", "icinga::Hello");
		message->Set("params", new Dictionary());
		JsonRpc::SendMessage(tlsStream, message);
		ctype = ClientJsonRpc;
	} else {
		tlsStream->WaitForData(5);

		if (!tlsStream->IsDataAvailable()) {
			Log(LogWarning, "ApiListener", "No data received on new API connection.");
			return;
		}

		char firstByte;
		tlsStream->Peek(&firstByte, 1, false);

		if (firstByte >= '0' && firstByte <= '9')
			ctype = ClientJsonRpc;
		else
			ctype = ClientHttp;
	}

	if (ctype == ClientJsonRpc) {
		Log(LogNotice, "ApiListener", "New JSON-RPC client");

		JsonRpcConnection::Ptr aclient = new JsonRpcConnection(identity, verify_ok, tlsStream, role);
		aclient->Start();

		if (endpoint) {
			bool needSync = !endpoint->GetConnected();

			endpoint->AddClient(aclient);

			m_SyncQueue.Enqueue(boost::bind(&ApiListener::SyncClient, this, aclient, endpoint, needSync));
		} else
			AddAnonymousClient(aclient);
	} else {
		Log(LogNotice, "ApiListener", "New HTTP client");

		HttpServerConnection::Ptr aclient = new HttpServerConnection(identity, verify_ok, tlsStream);
		aclient->Start();
		AddHttpClient(aclient);
	}
}

void ApiListener::SyncClient(const JsonRpcConnection::Ptr& aclient, const Endpoint::Ptr& endpoint, bool needSync)
{
	try {
		{
			ObjectLock olock(endpoint);

			endpoint->SetSyncing(true);
		}

		/* Make sure that the config updates are synced
		 * before the logs are replayed.
		 */

		Log(LogInformation, "ApiListener")
		    << "Sending config updates for endpoint '" << endpoint->GetName() << "'.";

		/* sync zone file config */
		SendConfigUpdate(aclient);
		/* sync runtime config */
		SendRuntimeConfigObjects(aclient);

		Log(LogInformation, "ApiListener")
		    << "Finished sending config updates for endpoint '" << endpoint->GetName() << "'.";

		if (!needSync) {
			ObjectLock olock2(endpoint);
			endpoint->SetSyncing(false);
			return;
		}

		Log(LogInformation, "ApiListener")
		    << "Sending replay log for endpoint '" << endpoint->GetName() << "'.";

		ReplayLog(aclient);

		Log(LogInformation, "ApiListener")
		    << "Finished sending replay log for endpoint '" << endpoint->GetName() << "'.";
	} catch (const std::exception& ex) {
		ObjectLock olock2(endpoint);
		endpoint->SetSyncing(false);

		Log(LogCritical, "ApiListener")
		    << "Error while syncing endpoint '" << endpoint->GetName() << "': " << DiagnosticInformation(ex);
	}
}

void ApiListener::ApiTimerHandler(void)
{
	double now = Utility::GetTime();

	std::vector<int> files;
	Utility::Glob(GetApiDir() + "log/*", boost::bind(&ApiListener::LogGlobHandler, boost::ref(files), _1), GlobFile);
	std::sort(files.begin(), files.end());

	BOOST_FOREACH(int ts, files) {
		bool need = false;

		BOOST_FOREACH(const Endpoint::Ptr& endpoint, ConfigType::GetObjectsByType<Endpoint>()) {
			if (endpoint == GetLocalEndpoint())
				continue;

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

	Zone::Ptr my_zone = Zone::GetLocalZone();

	BOOST_FOREACH(const Zone::Ptr& zone, ConfigType::GetObjectsByType<Zone>()) {
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

		BOOST_FOREACH(const Endpoint::Ptr& endpoint, zone->GetEndpoints()) {
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

			boost::thread thread(boost::bind(&ApiListener::AddConnection, this, endpoint));
			thread.detach();
		}
	}

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, ConfigType::GetObjectsByType<Endpoint>()) {
		if (!endpoint->GetConnected())
			continue;

		double ts = endpoint->GetRemoteLogPosition();

		if (ts == 0)
			continue;

		Dictionary::Ptr lparams = new Dictionary();
		lparams->Set("log_position", ts);

		Dictionary::Ptr lmessage = new Dictionary();
		lmessage->Set("jsonrpc", "2.0");
		lmessage->Set("method", "log::SetLogPosition");
		lmessage->Set("params", lparams);

		double maxTs = 0;

		BOOST_FOREACH(const JsonRpcConnection::Ptr& client, endpoint->GetClients()) {
			if (client->GetTimestamp() > maxTs)
				maxTs = client->GetTimestamp();
		}

		BOOST_FOREACH(const JsonRpcConnection::Ptr& client, endpoint->GetClients()) {
			if (client->GetTimestamp() != maxTs)
				client->Disconnect();
			else
				client->SendMessage(lmessage);
		}

		Log(LogNotice, "ApiListener")
		    << "Setting log position for identity '" << endpoint->GetName() << "': "
		    << Utility::FormatDateTime("%Y/%m/%d %H:%M:%S", ts);
	}

	Endpoint::Ptr master = GetMaster();

	if (master)
		Log(LogNotice, "ApiListener")
		    << "Current zone master: " << master->GetName();

	std::vector<String> names;
	BOOST_FOREACH(const Endpoint::Ptr& endpoint, ConfigType::GetObjectsByType<Endpoint>())
		if (endpoint->GetConnected())
			names.push_back(endpoint->GetName() + " (" + Convert::ToString(endpoint->GetClients().size()) + ")");

	Log(LogNotice, "ApiListener")
	    << "Connected endpoints: " << Utility::NaturalJoin(names);
}

void ApiListener::RelayMessage(const MessageOrigin::Ptr& origin,
    const ConfigObject::Ptr& secobj, const Dictionary::Ptr& message, bool log)
{
	if (!IsActive())
		return;

	m_RelayQueue.Enqueue(boost::bind(&ApiListener::SyncRelayMessage, this, origin, secobj, message, log), PriorityNormal, true);
}

void ApiListener::PersistMessage(const Dictionary::Ptr& message, const ConfigObject::Ptr& secobj)
{
	double ts = message->Get("ts");

	ASSERT(ts != 0);

	Dictionary::Ptr pmessage = new Dictionary();
	pmessage->Set("timestamp", ts);

	pmessage->Set("message", JsonEncode(message));
	Dictionary::Ptr secname = new Dictionary();
	secname->Set("type", secobj->GetType()->GetName());
	secname->Set("name", secobj->GetName());
	pmessage->Set("secobj", secname);

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
		    << "Sending message to '" << endpoint->GetName() << "'";

		double maxTs = 0;

		BOOST_FOREACH(const JsonRpcConnection::Ptr& client, endpoint->GetClients()) {
			if (client->GetTimestamp() > maxTs)
				maxTs = client->GetTimestamp();
		}

		BOOST_FOREACH(const JsonRpcConnection::Ptr& client, endpoint->GetClients()) {
			if (client->GetTimestamp() != maxTs)
				continue;

			client->SendMessage(message);
		}
	}
}

bool ApiListener::RelayMessageOne(const Zone::Ptr& targetZone, const MessageOrigin::Ptr& origin, const Dictionary::Ptr& message, const Endpoint::Ptr& currentMaster)
{
	ASSERT(targetZone);

	Zone::Ptr myZone = Zone::GetLocalZone();

	/* only relay the message to a) the same zone, b) the parent zone and c) direct child zones */
	if (targetZone != myZone && targetZone != myZone->GetParent() && targetZone->GetParent() != myZone)
		return true;

	Endpoint::Ptr myEndpoint = GetLocalEndpoint();

	std::vector<Endpoint::Ptr> skippedEndpoints;

	bool relayed = false, log_needed = false, log_done = false;

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, targetZone->GetEndpoints()) {
		/* don't relay messages to ourselves */
		if (endpoint == GetLocalEndpoint())
			continue;

		log_needed = true;

		/* don't relay messages to disconnected endpoints */
		if (!endpoint->GetConnected()) {
			if (targetZone == myZone)
				log_done = false;

			continue;
		}

		log_done = true;

		/* don't relay the message to the zone through more than one endpoint unless this is our own zone */
		if (relayed && targetZone != myZone) {
			skippedEndpoints.push_back(endpoint);
			continue;
		}

		/* don't relay messages back to the endpoint which we got the message from */
		if (origin && origin->FromClient && endpoint == origin->FromClient->GetEndpoint()) {
			skippedEndpoints.push_back(endpoint);
			continue;
		}

		/* don't relay messages back to the zone which we got the message from */
		if (origin && origin->FromZone && targetZone == origin->FromZone) {
			skippedEndpoints.push_back(endpoint);
			continue;
		}

		/* only relay message to the master if we're not currently the master */
		if (currentMaster != myEndpoint && currentMaster != endpoint) {
			skippedEndpoints.push_back(endpoint);
			continue;
		}

		relayed = true;

		SyncSendMessage(endpoint, message);
	}

	if (!skippedEndpoints.empty()) {
		double ts = message->Get("ts");

		BOOST_FOREACH(const Endpoint::Ptr& endpoint, skippedEndpoints)
			endpoint->SetLocalLogPosition(ts);
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

	BOOST_FOREACH(const Zone::Ptr& zone, target_zone->GetAllParents()) {
		if (!RelayMessageOne(zone, origin, message, master))
			need_log = true;
	}

	if (log && need_log)
		PersistMessage(message, secobj);
}

String ApiListener::GetApiDir(void)
{
	return Application::GetLocalStateDir() + "/lib/icinga2/api/";
}

/* must hold m_LogLock */
void ApiListener::OpenLogFile(void)
{
	String path = GetApiDir() + "log/current";

	std::fstream *fp = new std::fstream(path.CStr(), std::fstream::out | std::ofstream::app);

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
void ApiListener::CloseLogFile(void)
{
	if (!m_LogFile)
		return;

	m_LogFile->Close();
	m_LogFile.reset();
}

/* must hold m_LogLock */
void ApiListener::RotateLogFile(void)
{
	double ts = GetLogMessageTimestamp();

	if (ts == 0)
		ts = Utility::GetTime();

	String oldpath = GetApiDir() + "log/current";
	String newpath = GetApiDir() + "log/" + Convert::ToString(static_cast<int>(ts)+1);
	(void) rename(oldpath.CStr(), newpath.CStr());
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
		RotateLogFile();

		if (count == -1 || count > 50000) {
			OpenLogFile();
			lock.unlock();
		} else {
			last_sync = true;
		}

		count = 0;

		std::vector<int> files;
		Utility::Glob(GetApiDir() + "log/*", boost::bind(&ApiListener::LogGlobHandler, boost::ref(files), _1), GlobFile);
		std::sort(files.begin(), files.end());

		BOOST_FOREACH(int ts, files) {
			String path = GetApiDir() + "log/" + Convert::ToString(ts);

			if (ts < peer_ts)
				continue;

			Log(LogNotice, "ApiListener")
			    << "Replaying log: " << path;

			std::fstream *fp = new std::fstream(path.CStr(), std::fstream::in | std::fstream::binary);
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
					    << "Unexpected end-of-file for cluster log: " << path;

					/* Log files may be incomplete or corrupted. This is perfectly OK. */
					break;
				}

				if (pmessage->Get("timestamp") <= peer_ts)
					continue;

				Dictionary::Ptr secname = pmessage->Get("secobj");

				if (secname) {
					ConfigType::Ptr dtype = ConfigType::GetByName(secname->Get("type"));

					if (!dtype)
						continue;

					ConfigObject::Ptr secobj = dtype->GetObject(secname->Get("name"));

					if (!secobj)
						continue;

					if (!target_zone->CanAccessObject(secobj))
						continue;
				}

				try  {
					NetString::WriteStringToStream(client->GetStream(), pmessage->Get("message"));
					count++;
				} catch (const std::exception& ex) {
					Log(LogWarning, "ApiListener")
					    << "Error while replaying log for endpoint '" << endpoint->GetName() << "': " << DiagnosticInformation(ex);
					break;
				}

				peer_ts = pmessage->Get("timestamp");

				if (ts > logpos_ts + 10) {
					logpos_ts = ts;

					Dictionary::Ptr lparams = new Dictionary();
					lparams->Set("log_position", logpos_ts);

					Dictionary::Ptr lmessage = new Dictionary();
					lmessage->Set("jsonrpc", "2.0");
					lmessage->Set("method", "log::SetLogPosition");
					lmessage->Set("params", lparams);

					JsonRpc::SendMessage(client->GetStream(), lmessage);
				}
			}

			logStream->Close();
		}

		if (count > 0) {
			Log(LogInformation, "ApiListener")
			   << "Replayed " << count << " messages.";
		}

		Log(LogNotice, "ApiListener")
		   << "Replayed " << count << " messages.";

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
	BOOST_FOREACH(const Dictionary::Pair& kv, stats.second)
		perfdata->Add("'api_" + kv.first + "'=" + Convert::ToString(kv.second));

	status->Set("api", stats.first);
}

std::pair<Dictionary::Ptr, Dictionary::Ptr> ApiListener::GetStatus(void)
{
	Dictionary::Ptr status = new Dictionary();
	Dictionary::Ptr perfdata = new Dictionary();

	/* cluster stats */
	status->Set("identity", GetIdentity());

	double allEndpoints = 0;
	Array::Ptr allNotConnectedEndpoints = new Array();
	Array::Ptr allConnectedEndpoints = new Array();

	Zone::Ptr my_zone = Zone::GetLocalZone();

	Dictionary::Ptr connectedZones = new Dictionary();

	BOOST_FOREACH(const Zone::Ptr& zone, ConfigType::GetObjectsByType<Zone>()) {
		/* only check endpoints in a) the same zone b) our parent zone c) immediate child zones */
		if (my_zone != zone && my_zone != zone->GetParent() && zone != my_zone->GetParent()) {
			Log(LogDebug, "ApiListener")
			    << "Not checking connection to Zone '" << zone->GetName() << "' because it's not in the same zone, a parent or a child zone.";
			continue;
		}

		bool zoneConnected = false;
		int countZoneEndpoints = 0;
		double zoneLag = 0;

		Array::Ptr zoneEndpoints = new Array();

		BOOST_FOREACH(const Endpoint::Ptr& endpoint, zone->GetEndpoints()) {
			zoneEndpoints->Add(endpoint->GetName());

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

		Dictionary::Ptr zoneStats = new Dictionary();
		zoneStats->Set("connected", zoneConnected);
		zoneStats->Set("client_log_lag", zoneLag);
		zoneStats->Set("endpoints", zoneEndpoints);

		String parentZoneName;
		Zone::Ptr parentZone = zone->GetParent();
		if (parentZone)
			parentZoneName = parentZone->GetName();

		zoneStats->Set("parent_zone", parentZoneName);

		connectedZones->Set(zone->GetName(), zoneStats);
	}

	status->Set("num_endpoints", allEndpoints);
	status->Set("num_conn_endpoints", allConnectedEndpoints->GetLength());
	status->Set("num_not_conn_endpoints", allNotConnectedEndpoints->GetLength());
	status->Set("conn_endpoints", allConnectedEndpoints);
	status->Set("not_conn_endpoints", allNotConnectedEndpoints);

	status->Set("zones", connectedZones);

	perfdata->Set("num_endpoints", allEndpoints);
	perfdata->Set("num_conn_endpoints", Convert::ToDouble(allConnectedEndpoints->GetLength()));
	perfdata->Set("num_not_conn_endpoints", Convert::ToDouble(allNotConnectedEndpoints->GetLength()));

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

void ApiListener::AddAnonymousClient(const JsonRpcConnection::Ptr& aclient)
{
	ObjectLock olock(this);
	m_AnonymousClients.insert(aclient);
}

void ApiListener::RemoveAnonymousClient(const JsonRpcConnection::Ptr& aclient)
{
	ObjectLock olock(this);
	m_AnonymousClients.erase(aclient);
}

std::set<JsonRpcConnection::Ptr> ApiListener::GetAnonymousClients(void) const
{
	ObjectLock olock(this);
	return m_AnonymousClients;
}

void ApiListener::AddHttpClient(const HttpServerConnection::Ptr& aclient)
{
	ObjectLock olock(this);
	m_HttpClients.insert(aclient);
}

void ApiListener::RemoveHttpClient(const HttpServerConnection::Ptr& aclient)
{
	ObjectLock olock(this);
	m_HttpClients.erase(aclient);
}

std::set<HttpServerConnection::Ptr> ApiListener::GetHttpClients(void) const
{
	ObjectLock olock(this);
	return m_HttpClients;
}

Value ApiListener::HelloAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params)
{
	return Empty;
}

Endpoint::Ptr ApiListener::GetLocalEndpoint(void) const
{
	return m_LocalEndpoint;
}
