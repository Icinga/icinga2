/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "base/convert.hpp"
#include "base/netstring.hpp"
#include "base/json.hpp"
#include "base/dynamictype.hpp"
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

REGISTER_STATSFUNCTION(ApiListenerStats, &ApiListener::StatsFunc);

ApiListener::ApiListener(void)
	: m_LogMessageCount(0)
{ }

void ApiListener::OnConfigLoaded(void)
{
	/* set up SSL context */
	boost::shared_ptr<X509> cert;
	try {
		cert = GetX509Certificate(GetCertPath());
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ScriptError("Cannot get certificate from cert path: '" + GetCertPath() + "'.", GetDebugInfo()));
	}

	try {
		SetIdentity(GetCertificateCN(cert));
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ScriptError("Cannot get certificate common name from cert path: '" + GetCertPath() + "'.", GetDebugInfo()));
	}

	Log(LogInformation, "ApiListener")
	    << "My API identity: " << GetIdentity();

	try {
		m_SSLContext = MakeSSLContext(GetCertPath(), GetKeyPath(), GetCaPath());
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ScriptError("Cannot make SSL context for cert path: '" + GetCertPath() + "' key path: '" + GetKeyPath() + "' ca path: '" + GetCaPath() + "'.", GetDebugInfo()));
	}

	if (!GetCrlPath().IsEmpty()) {
		try {
			AddCRLToSSLContext(m_SSLContext, GetCrlPath());
		} catch (const std::exception&) {
			BOOST_THROW_EXCEPTION(ScriptError("Cannot add certificate revocation list to SSL context for crl path: '" + GetCrlPath() + "'.", GetDebugInfo()));
		}
	}
}

void ApiListener::OnAllConfigLoaded(void)
{
	if (!Endpoint::GetByName(GetIdentity()))
		BOOST_THROW_EXCEPTION(ScriptError("Endpoint object for '" + GetIdentity() + "' is missing.", GetDebugInfo()));
}

/**
 * Starts the component.
 */
void ApiListener::Start(void)
{
	SyncZoneDirs();

	if (std::distance(DynamicType::GetObjectsByType<ApiListener>().first, DynamicType::GetObjectsByType<ApiListener>().second) > 1) {
		Log(LogCritical, "ApiListener", "Only one ApiListener object is allowed.");
		return;
	}

	DynamicObject::Start();

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
	BOOST_FOREACH(const ApiListener::Ptr& listener, DynamicType::GetObjectsByType<ApiListener>())
		return listener;

	return ApiListener::Ptr();
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
		if (endpoint->IsConnected() || endpoint->GetName() == GetIdentity())
			names.push_back(endpoint->GetName());

	std::sort(names.begin(), names.end());

	return Endpoint::GetByName(*names.begin());
}

bool ApiListener::IsMaster(void) const
{
	Endpoint::Ptr master = GetMaster();

	if (!master)
		return false;

	return master->GetName() == GetIdentity();
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

/**
 * Processes a new client connection.
 *
 * @param client The new client.
 */
void ApiListener::NewClientHandler(const Socket::Ptr& client, const String& hostname, ConnectionRole role)
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

		Log(LogInformation, "ApiListener")
		    << "New client connection for identity '" << identity << "'" << (verify_ok ? "" : " (unauthenticated)");


		if (verify_ok)
			endpoint = Endpoint::GetByName(identity);
	} else {
		Log(LogInformation, "ApiListener")
		    << "New client connection (no client certificate)";
	}

	bool need_sync = false;

	if (endpoint)
		need_sync = !endpoint->IsConnected();

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
		Log(LogInformation, "ApiListener", "New JSON-RPC client");

		JsonRpcConnection::Ptr aclient = new JsonRpcConnection(identity, verify_ok, tlsStream, role);
		aclient->Start();

		if (endpoint) {
			endpoint->AddClient(aclient);

			if (need_sync) {
				{
					ObjectLock olock(endpoint);

					endpoint->SetSyncing(true);
				}

				ReplayLog(aclient);
			}

			SendConfigUpdate(aclient);
		} else
			AddAnonymousClient(aclient);
	} else {
		Log(LogInformation, "ApiListener", "New HTTP client");

		HttpConnection::Ptr aclient = new HttpConnection(identity, verify_ok, tlsStream);
		aclient->Start();
		AddHttpClient(aclient);
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

		BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjectsByType<Endpoint>()) {
			if (endpoint->GetName() == GetIdentity())
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

	BOOST_FOREACH(const Zone::Ptr& zone, DynamicType::GetObjectsByType<Zone>()) {
		/* only connect to endpoints in a) the same zone b) our parent zone c) immediate child zones */
		if (my_zone != zone && my_zone != zone->GetParent() && zone != my_zone->GetParent()) {
			Log(LogDebug, "ApiListener")
			    << "Not connecting to Zone '" << zone->GetName() << "' because it's not in the same zone, a parent or a child zone.";
			continue;
		}

		BOOST_FOREACH(const Endpoint::Ptr& endpoint, zone->GetEndpoints()) {
			/* don't connect to ourselves */
			if (endpoint->GetName() == GetIdentity()) {
				Log(LogDebug, "ApiListener")
				    << "Not connecting to Endpoint '" << endpoint->GetName() << "' because that's us.";
				continue;
			}

			/* don't try to connect to endpoints which don't have a host and port */
			if (endpoint->GetHost().IsEmpty() || endpoint->GetPort().IsEmpty()) {
				Log(LogDebug, "ApiListener")
				    << "Not connecting to Endpoint '" << endpoint->GetName() << "' because the host/port attributes are missing.";
				continue;
			}

			/* don't try to connect if there's already a connection attempt */
			if (endpoint->GetConnecting()) {
				Log(LogDebug, "ApiListener")
				    << "Not connecting to Endpoint '" << endpoint->GetName() << "' because we're already trying to connect to it.";
				continue;
			}

			/* don't try to connect if we're already connected */
			if (endpoint->IsConnected()) {
				Log(LogDebug, "ApiListener")
				    << "Not connecting to Endpoint '" << endpoint->GetName() << "' because we're already connected to it.";
				continue;
			}

			boost::thread thread(boost::bind(&ApiListener::AddConnection, this, endpoint));
			thread.detach();
		}
	}

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjectsByType<Endpoint>()) {
		if (!endpoint->IsConnected())
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

		BOOST_FOREACH(const JsonRpcConnection::Ptr& client, endpoint->GetClients())
			client->SendMessage(lmessage);

		Log(LogNotice, "ApiListener")
		    << "Setting log position for identity '" << endpoint->GetName() << "': "
		    << Utility::FormatDateTime("%Y/%m/%d %H:%M:%S", ts);
	}

	Endpoint::Ptr master = GetMaster();

	if (master)
		Log(LogNotice, "ApiListener")
		    << "Current zone master: " << master->GetName();

	std::vector<String> names;
	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjectsByType<Endpoint>())
		if (endpoint->IsConnected())
			names.push_back(endpoint->GetName() + " (" + Convert::ToString(endpoint->GetClients().size()) + ")");

	Log(LogNotice, "ApiListener")
	    << "Connected endpoints: " << Utility::NaturalJoin(names);
}

void ApiListener::RelayMessage(const MessageOrigin& origin, const DynamicObject::Ptr& secobj, const Dictionary::Ptr& message, bool log)
{
	m_RelayQueue.Enqueue(boost::bind(&ApiListener::SyncRelayMessage, this, origin, secobj, message, log));
}

void ApiListener::PersistMessage(const Dictionary::Ptr& message, const DynamicObject::Ptr& secobj)
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

		BOOST_FOREACH(const JsonRpcConnection::Ptr& client, endpoint->GetClients())
			client->SendMessage(message);
	}
}


void ApiListener::SyncRelayMessage(const MessageOrigin& origin, const DynamicObject::Ptr& secobj, const Dictionary::Ptr& message, bool log)
{
	double ts = Utility::GetTime();
	message->Set("ts", ts);

	Log(LogNotice, "ApiListener")
	    << "Relaying '" << message->Get("method") << "' message";

	if (log)
		PersistMessage(message, secobj);

	if (origin.FromZone)
		message->Set("originZone", origin.FromZone->GetName());

	bool is_master = IsMaster();
	Endpoint::Ptr master = GetMaster();
	Zone::Ptr my_zone = Zone::GetLocalZone();

	std::vector<Endpoint::Ptr> skippedEndpoints;
	std::set<Zone::Ptr> finishedZones;

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjectsByType<Endpoint>()) {
		/* don't relay messages to ourselves or disconnected endpoints */
		if (endpoint->GetName() == GetIdentity() || !endpoint->IsConnected())
			continue;

		Zone::Ptr target_zone = endpoint->GetZone();

		/* don't relay the message to the zone through more than one endpoint */
		if (finishedZones.find(target_zone) != finishedZones.end()) {
			skippedEndpoints.push_back(endpoint);
			continue;
		}

		/* don't relay messages back to the endpoint which we got the message from */
		if (origin.FromClient && endpoint == origin.FromClient->GetEndpoint()) {
			skippedEndpoints.push_back(endpoint);
			continue;
		}

		/* don't relay messages back to the zone which we got the message from */
		if (origin.FromZone && target_zone == origin.FromZone) {
			skippedEndpoints.push_back(endpoint);
			continue;
		}

		/* only relay message to the master if we're not currently the master */
		if (!is_master && master != endpoint) {
			skippedEndpoints.push_back(endpoint);
			continue;
		}

		/* only relay the message to a) the same zone, b) the parent zone and c) direct child zones */
		if (target_zone != my_zone && target_zone != my_zone->GetParent() &&
		    secobj->GetZoneName() != target_zone->GetName()) {
			skippedEndpoints.push_back(endpoint);
			continue;
		}

		/* only relay messages to zones which have access to the object */
		if (!target_zone->CanAccessObject(secobj))
			continue;

		finishedZones.insert(target_zone);

		SyncSendMessage(endpoint, message);
	}

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, skippedEndpoints)
		endpoint->SetLocalLogPosition(ts);
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

	int ts;

	try {
		ts = Convert::ToLong(name);
	}
	catch (const std::exception&) {
		return;
	}

	files.push_back(ts);
}

void ApiListener::ReplayLog(const JsonRpcConnection::Ptr& client)
{
	Endpoint::Ptr endpoint = client->GetEndpoint();

	CONTEXT("Replaying log for Endpoint '" + endpoint->GetName() + "'");

	int count = -1;
	double peer_ts = endpoint->GetLocalLogPosition();
	double logpos_ts = peer_ts;
	bool last_sync = false;
	
	Endpoint::Ptr target_endpoint = client->GetEndpoint();
	ASSERT(target_endpoint);
	
	Zone::Ptr target_zone = target_endpoint->GetZone();
	
	if (!target_zone)
		return;

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
					DynamicType::Ptr dtype = DynamicType::GetByName(secname->Get("type"));
					
					if (!dtype)
						continue;
					
					DynamicObject::Ptr secobj = dtype->GetObject(secname->Get("name"));
					
					if (!secobj)
						continue;
					
					if (!target_zone->CanAccessObject(secobj))
						continue;
				}

				NetString::WriteStringToStream(client->GetStream(), pmessage->Get("message"));
				count++;

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
	Dictionary::Ptr nodes = new Dictionary();
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

	double count_endpoints = 0;
	Array::Ptr not_connected_endpoints = new Array();
	Array::Ptr connected_endpoints = new Array();

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjectsByType<Endpoint>()) {
		if (endpoint->GetName() == GetIdentity())
			continue;

		count_endpoints++;

		if (!endpoint->IsConnected())
			not_connected_endpoints->Add(endpoint->GetName());
		else
			connected_endpoints->Add(endpoint->GetName());
	}

	status->Set("num_endpoints", count_endpoints);
	status->Set("num_conn_endpoints", connected_endpoints->GetLength());
	status->Set("num_not_conn_endpoints", not_connected_endpoints->GetLength());
	status->Set("conn_endpoints", connected_endpoints);
	status->Set("not_conn_endpoints", not_connected_endpoints);

	perfdata->Set("num_endpoints", count_endpoints);
	perfdata->Set("num_conn_endpoints", Convert::ToDouble(connected_endpoints->GetLength()));
	perfdata->Set("num_not_conn_endpoints", Convert::ToDouble(not_connected_endpoints->GetLength()));

	return std::make_pair(status, perfdata);
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

void ApiListener::AddHttpClient(const HttpConnection::Ptr& aclient)
{
	ObjectLock olock(this);
	m_HttpClients.insert(aclient);
}

void ApiListener::RemoveHttpClient(const HttpConnection::Ptr& aclient)
{
	ObjectLock olock(this);
	m_HttpClients.erase(aclient);
}

std::set<HttpConnection::Ptr> ApiListener::GetHttpClients(void) const
{
	ObjectLock olock(this);
	return m_HttpClients;
}
