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

#include "remote/apilistener.hpp"
#include "remote/apiclient.hpp"
#include "remote/endpoint.hpp"
#include "base/convert.hpp"
#include "base/netstring.hpp"
#include "base/dynamictype.hpp"
#include "base/logger_fwd.hpp"
#include "base/objectlock.hpp"
#include "base/stdiostream.hpp"
#include "base/application.hpp"
#include "base/context.hpp"
#include "base/statsfunction.hpp"
#include <fstream>

using namespace icinga;

REGISTER_TYPE(ApiListener);

boost::signals2::signal<void(bool)> ApiListener::OnMasterChanged;

REGISTER_STATSFUNCTION(ApiListenerStats, &ApiListener::StatsFunc);

void ApiListener::OnConfigLoaded(void)
{
	/* set up SSL context */
	shared_ptr<X509> cert = GetX509Certificate(GetCertPath());
	SetIdentity(GetCertificateCN(cert));
	Log(LogInformation, "ApiListener", "My API identity: " + GetIdentity());

	m_SSLContext = MakeSSLContext(GetCertPath(), GetKeyPath(), GetCaPath());

	if (!GetCrlPath().IsEmpty())
		AddCRLToSSLContext(m_SSLContext, GetCrlPath());

	if (!Endpoint::GetByName(GetIdentity()))
		BOOST_THROW_EXCEPTION(std::runtime_error("Endpoint object for '" + GetIdentity() + "' is missing."));

	SyncZoneDirs();
}

/**
 * Starts the component.
 */
void ApiListener::Start(void)
{
	if (std::distance(DynamicType::GetObjects<ApiListener>().first, DynamicType::GetObjects<ApiListener>().second) > 1)
		BOOST_THROW_EXCEPTION(std::runtime_error("Only one ApiListener object is allowed."));

	DynamicObject::Start();

	{
		boost::mutex::scoped_lock(m_LogLock);
		RotateLogFile();
		OpenLogFile();
	}

	/* create the primary JSON-RPC listener */
	AddListener(GetBindPort());

	m_Timer = make_shared<Timer>();
	m_Timer->OnTimerExpired.connect(boost::bind(&ApiListener::ApiTimerHandler, this));
	m_Timer->SetInterval(5);
	m_Timer->Start();
	m_Timer->Reschedule(0);

	OnMasterChanged(true);
}

ApiListener::Ptr ApiListener::GetInstance(void)
{
	BOOST_FOREACH(const ApiListener::Ptr& listener, DynamicType::GetObjects<ApiListener>())
		return listener;

	return ApiListener::Ptr();
}

shared_ptr<SSL_CTX> ApiListener::GetSSLContext(void) const
{
	return m_SSLContext;
}

Endpoint::Ptr ApiListener::GetMaster(void) const
{
	Zone::Ptr zone = Zone::GetLocalZone();
	std::vector<String> names;

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, zone->GetEndpoints())
		if (endpoint->IsConnected() || endpoint->GetName() == GetIdentity())
			names.push_back(endpoint->GetName());

	std::sort(names.begin(), names.end());

	return Endpoint::GetByName(*names.begin());
}

bool ApiListener::IsMaster(void) const
{
	return GetMaster()->GetName() == GetIdentity();
}

/**
 * Creates a new JSON-RPC listener on the specified port.
 *
 * @param service The port to listen on.
 */
void ApiListener::AddListener(const String& service)
{
	ObjectLock olock(this);

	shared_ptr<SSL_CTX> sslContext = m_SSLContext;

	if (!sslContext)
		BOOST_THROW_EXCEPTION(std::logic_error("SSL context is required for AddListener()"));

	std::ostringstream s;
	s << "Adding new listener: port " << service;
	Log(LogInformation, "ApiListener", s.str());

	TcpSocket::Ptr server = make_shared<TcpSocket>();
	server->Bind(service, AF_UNSPEC);

	boost::thread thread(boost::bind(&ApiListener::ListenerThreadProc, this, server));
	thread.detach();

	m_Servers.insert(server);
}

void ApiListener::ListenerThreadProc(const Socket::Ptr& server)
{
	Utility::SetThreadName("API Listener");

	server->Listen();

	for (;;) {
		Socket::Ptr client = server->Accept();

		Utility::QueueAsyncCallback(boost::bind(&ApiListener::NewClientHandler, this, client, RoleServer));
	}
}

/**
 * Creates a new JSON-RPC client and connects to the specified host and port.
 *
 * @param node The remote host.
 * @param service The remote port.
 */
void ApiListener::AddConnection(const String& node, const String& service)
{
	{
		ObjectLock olock(this);

		shared_ptr<SSL_CTX> sslContext = m_SSLContext;

		if (!sslContext)
			BOOST_THROW_EXCEPTION(std::logic_error("SSL context is required for AddConnection()"));
	}

	TcpSocket::Ptr client = make_shared<TcpSocket>();

	try {
		client->Connect(node, service);
		Utility::QueueAsyncCallback(boost::bind(&ApiListener::NewClientHandler, this, client, RoleClient));
	} catch (const std::exception& ex) {
		std::ostringstream info, debug;
		info << "Cannot connect to host '" << node << "' on port '" << service << "'";
		debug << info.str() << std::endl << DiagnosticInformation(ex);
		Log(LogCritical, "remote", info.str());
		Log(LogDebug, "ApiListener", debug.str());
	}
}

/**
 * Processes a new client connection.
 *
 * @param client The new client.
 */
void ApiListener::NewClientHandler(const Socket::Ptr& client, ConnectionRole role)
{
	CONTEXT("Handling new API client connection");

	TlsStream::Ptr tlsStream;

	{
		ObjectLock olock(this);
		tlsStream = make_shared<TlsStream>(client, role, m_SSLContext);
	}

	tlsStream->Handshake();

	shared_ptr<X509> cert = tlsStream->GetPeerCertificate();
	String identity = GetCertificateCN(cert);

	Log(LogInformation, "ApiListener", "New client connection for identity '" + identity + "'");

	Endpoint::Ptr endpoint = Endpoint::GetByName(identity);

	bool need_sync = false;

	if (endpoint)
		need_sync = !endpoint->IsConnected();

	ApiClient::Ptr aclient = make_shared<ApiClient>(identity, tlsStream, role);
	aclient->Start();

	if (endpoint) {
		if (need_sync) {
			{
				ObjectLock olock(endpoint);

				endpoint->SetSyncing(true);
			}

			ReplayLog(aclient);
		}

		SendConfigUpdate(aclient);

		endpoint->AddClient(aclient);
	} else
		AddAnonymousClient(aclient);
}

void ApiListener::ApiTimerHandler(void)
{
	double now = Utility::GetTime();

	std::vector<int> files;
	Utility::Glob(GetApiDir() + "log/*", boost::bind(&ApiListener::LogGlobHandler, boost::ref(files), _1), GlobFile);
	std::sort(files.begin(), files.end());

	BOOST_FOREACH(int ts, files) {
		bool need = false;

		BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
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
			Log(LogNotice, "ApiListener", "Removing old log file: " + path);
			(void)unlink(path.CStr());
		}
	}

	if (IsMaster()) {
		Zone::Ptr my_zone = Zone::GetLocalZone();

		BOOST_FOREACH(const Zone::Ptr& zone, DynamicType::GetObjects<Zone>()) {
			/* only connect to endpoints in a) the same zone b) our parent zone c) immediate child zones */
			if (my_zone != zone && my_zone != zone->GetParent() && zone != my_zone->GetParent())
				continue;

			bool connected = false;

			BOOST_FOREACH(const Endpoint::Ptr& endpoint, zone->GetEndpoints()) {
				if (endpoint->IsConnected()) {
					connected = true;
					break;
				}
			}

			/* don't connect to an endpoint if we already have a connection to the zone */
			if (connected)
				continue;

			BOOST_FOREACH(const Endpoint::Ptr& endpoint, zone->GetEndpoints()) {
				/* don't connect to ourselves */
				if (endpoint->GetName() == GetIdentity())
					continue;

				/* don't try to connect to endpoints which don't have a host and port */
				if (endpoint->GetHost().IsEmpty() || endpoint->GetPort().IsEmpty())
					continue;

				AddConnection(endpoint->GetHost(), endpoint->GetPort());
			}
		}
	}

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
		if (!endpoint->IsConnected())
			continue;

		double ts = endpoint->GetRemoteLogPosition();

		if (ts == 0)
			continue;

		Dictionary::Ptr lparams = make_shared<Dictionary>();
		lparams->Set("log_position", ts);

		Dictionary::Ptr lmessage = make_shared<Dictionary>();
		lmessage->Set("jsonrpc", "2.0");
		lmessage->Set("method", "log::SetLogPosition");
		lmessage->Set("params", lparams);

		BOOST_FOREACH(const ApiClient::Ptr& client, endpoint->GetClients())
			client->SendMessage(lmessage);

		Log(LogNotice, "ApiListener", "Setting log position for identity '" + endpoint->GetName() + "': " +
			Utility::FormatDateTime("%Y/%m/%d %H:%M:%S", ts));
	}

	Log(LogNotice, "ApiListener", "Current zone master: " + GetMaster()->GetName());

	std::vector<String> names;
	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>())
		if (endpoint->IsConnected())
			names.push_back(endpoint->GetName() + " (" + Convert::ToString(endpoint->GetClients().size()) + ")");

	Log(LogNotice, "ApiListener", "Connected endpoints: " + Utility::NaturalJoin(names));
}

void ApiListener::RelayMessage(const MessageOrigin& origin, const DynamicObject::Ptr& secobj, const Dictionary::Ptr& message, bool log)
{
	m_RelayQueue.Enqueue(boost::bind(&ApiListener::SyncRelayMessage, this, origin, secobj, message, log));
}

void ApiListener::PersistMessage(const Dictionary::Ptr& message)
{
	double ts = message->Get("ts");

	ASSERT(ts != 0);

	Dictionary::Ptr pmessage = make_shared<Dictionary>();
	pmessage->Set("timestamp", ts);

	pmessage->Set("message", JsonSerialize(message));

	boost::mutex::scoped_lock lock(m_LogLock);
	if (m_LogFile) {
		NetString::WriteStringToStream(m_LogFile, JsonSerialize(pmessage));
		m_LogMessageCount++;
		SetLogMessageTimestamp(ts);

		if (m_LogMessageCount > 50000) {
			CloseLogFile();
			RotateLogFile();
			OpenLogFile();
		}
	}
}

void ApiListener::SyncRelayMessage(const MessageOrigin& origin, const DynamicObject::Ptr& secobj, const Dictionary::Ptr& message, bool log)
{
	double ts = Utility::GetTime();
	message->Set("ts", ts);

	Log(LogNotice, "ApiListener", "Relaying '" + message->Get("method") + "' message");

	if (log)
		m_LogQueue.Enqueue(boost::bind(&ApiListener::PersistMessage, this, message));

	if (origin.FromZone)
		message->Set("originZone", origin.FromZone->GetName());

	bool is_master = IsMaster();
	Endpoint::Ptr master = GetMaster();
	Zone::Ptr my_zone = Zone::GetLocalZone();

	std::vector<Endpoint::Ptr> skippedEndpoints;
	std::set<Zone::Ptr> finishedZones;

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
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
		    secobj->GetZone() != target_zone->GetName()) {
			skippedEndpoints.push_back(endpoint);
			continue;
		}

		/* only relay messages to zones which have access to the object */
		if (!target_zone->CanAccessObject(secobj))
			continue;

		finishedZones.insert(target_zone);

		{
			ObjectLock olock(endpoint);

			if (!endpoint->GetSyncing()) {
				Log(LogNotice, "ApiListener", "Sending message to '" + endpoint->GetName() + "'");

				BOOST_FOREACH(const ApiClient::Ptr& client, endpoint->GetClients())
					client->SendMessage(message);
			}
		}
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
		Log(LogWarning, "ApiListener", "Could not open spool file: " + path);
		return;
	}

	m_LogFile = make_shared<StdioStream>(fp, true);
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

void ApiListener::ReplayLog(const ApiClient::Ptr& client)
{
	Endpoint::Ptr endpoint = client->GetEndpoint();

	CONTEXT("Replaying log for Endpoint '" + endpoint->GetName() + "'");

	int count = -1;
	double peer_ts = endpoint->GetLocalLogPosition();
	bool last_sync = false;

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

			Log(LogNotice, "ApiListener", "Replaying log: " + path);

			std::fstream *fp = new std::fstream(path.CStr(), std::fstream::in);
			StdioStream::Ptr logStream = make_shared<StdioStream>(fp, true);

			String message;
			while (true) {
				Dictionary::Ptr pmessage;

				try {
					if (!NetString::ReadStringFromStream(logStream, &message))
						break;

					pmessage = JsonDeserialize(message);
				} catch (const std::exception&) {
					Log(LogWarning, "ApiListener", "Unexpected end-of-file for cluster log: " + path);

					/* Log files may be incomplete or corrupted. This is perfectly OK. */
					break;
				}

				if (pmessage->Get("timestamp") <= peer_ts)
					continue;

				NetString::WriteStringToStream(client->GetStream(), pmessage->Get("message"));
				count++;

				peer_ts = pmessage->Get("timestamp");
			}

			logStream->Close();
		}

		Log(LogNotice, "ApiListener", "Replayed " + Convert::ToString(count) + " messages.");

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

Value ApiListener::StatsFunc(Dictionary::Ptr& status, Dictionary::Ptr& perfdata)
{
	Dictionary::Ptr nodes = make_shared<Dictionary>();
	std::pair<Dictionary::Ptr, Dictionary::Ptr> stats;

	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return 0;

	stats = listener->GetStatus();

	BOOST_FOREACH(Dictionary::Pair const& kv, stats.second)
		perfdata->Set("api_" + kv.first, kv.second);

	status->Set("api", stats.first);

	return 0;
}

std::pair<Dictionary::Ptr, Dictionary::Ptr> ApiListener::GetStatus(void)
{
	Dictionary::Ptr status = make_shared<Dictionary>();
	Dictionary::Ptr perfdata = make_shared<Dictionary>();

	/* cluster stats */
	status->Set("identity", GetIdentity());

	double count_endpoints = 0;
	Array::Ptr not_connected_endpoints = make_shared<Array>();
	Array::Ptr connected_endpoints = make_shared<Array>();

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
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

void ApiListener::AddAnonymousClient(const ApiClient::Ptr& aclient)
{
	ObjectLock olock(this);
	m_AnonymousClients.insert(aclient);
}

void ApiListener::RemoveAnonymousClient(const ApiClient::Ptr& aclient)
{
	ObjectLock olock(this);
	m_AnonymousClients.erase(aclient);
}

std::set<ApiClient::Ptr> ApiListener::GetAnonymousClients(void) const
{
	ObjectLock olock(this);
	return m_AnonymousClients;
}
