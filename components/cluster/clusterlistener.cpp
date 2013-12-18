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

#include "cluster/clusterlistener.h"
#include "cluster/endpoint.h"
#include "icinga/domain.h"
#include "icinga/icingaapplication.h"
#include "base/netstring.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/objectlock.h"
#include "base/networkstream.h"
#include "base/zlibstream.h"
#include "base/application.h"
#include "base/convert.h"
#include "base/context.h"
#include <fstream>

using namespace icinga;

REGISTER_TYPE(ClusterListener);

/**
 * Starts the component.
 */
void ClusterListener::Start(void)
{
	DynamicObject::Start();

	{
		ObjectLock olock(this);
		RotateLogFile();
		OpenLogFile();
	}

	/* set up SSL context */
	shared_ptr<X509> cert = GetX509Certificate(GetCertPath());
	SetIdentity(GetCertificateCN(cert));
	Log(LogInformation, "cluster", "My identity: " + GetIdentity());

	Endpoint::Ptr self = Endpoint::GetByName(GetIdentity());

	if (!self)
		BOOST_THROW_EXCEPTION(std::invalid_argument("No configuration available for the local endpoint."));

	m_SSLContext = MakeSSLContext(GetCertPath(), GetKeyPath(), GetCaPath());

	if (!GetCrlPath().IsEmpty())
		AddCRLToSSLContext(m_SSLContext, GetCrlPath());

	/* create the primary JSON-RPC listener */
	if (!GetBindPort().IsEmpty())
		AddListener(GetBindPort());

	m_ClusterTimer = make_shared<Timer>();
	m_ClusterTimer->OnTimerExpired.connect(boost::bind(&ClusterListener::ClusterTimerHandler, this));
	m_ClusterTimer->SetInterval(5);
	m_ClusterTimer->Start();

	m_MessageQueue.SetExceptionCallback(&ClusterListener::MessageExceptionHandler);

	Service::OnNewCheckResult.connect(boost::bind(&ClusterListener::CheckResultHandler, this, _1, _2, _3));
	Service::OnNextCheckChanged.connect(boost::bind(&ClusterListener::NextCheckChangedHandler, this, _1, _2, _3));
	Notification::OnNextNotificationChanged.connect(boost::bind(&ClusterListener::NextNotificationChangedHandler, this, _1, _2, _3));
	Service::OnForceNextCheckChanged.connect(boost::bind(&ClusterListener::ForceNextCheckChangedHandler, this, _1, _2, _3));
	Service::OnForceNextNotificationChanged.connect(boost::bind(&ClusterListener::ForceNextNotificationChangedHandler, this, _1, _2, _3));
	Service::OnEnableActiveChecksChanged.connect(boost::bind(&ClusterListener::EnableActiveChecksChangedHandler, this, _1, _2, _3));
	Service::OnEnablePassiveChecksChanged.connect(boost::bind(&ClusterListener::EnablePassiveChecksChangedHandler, this, _1, _2, _3));
	Service::OnEnableNotificationsChanged.connect(boost::bind(&ClusterListener::EnableNotificationsChangedHandler, this, _1, _2, _3));
	Service::OnEnableFlappingChanged.connect(boost::bind(&ClusterListener::EnableFlappingChangedHandler, this, _1, _2, _3));
	Service::OnCommentAdded.connect(boost::bind(&ClusterListener::CommentAddedHandler, this, _1, _2, _3));
	Service::OnCommentRemoved.connect(boost::bind(&ClusterListener::CommentRemovedHandler, this, _1, _2, _3));
	Service::OnDowntimeAdded.connect(boost::bind(&ClusterListener::DowntimeAddedHandler, this, _1, _2, _3));
	Service::OnDowntimeRemoved.connect(boost::bind(&ClusterListener::DowntimeRemovedHandler, this, _1, _2, _3));
	Service::OnAcknowledgementSet.connect(boost::bind(&ClusterListener::AcknowledgementSetHandler, this, _1, _2, _3, _4, _5, _6));
	Service::OnAcknowledgementCleared.connect(boost::bind(&ClusterListener::AcknowledgementClearedHandler, this, _1, _2));

	Endpoint::OnMessageReceived.connect(boost::bind(&ClusterListener::AsyncMessageHandler, this, _1, _2));

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
				int privs = 0;

				Array::Ptr domains = object->GetDomains();

				if (domains) {
					ObjectLock olock(domains);
					BOOST_FOREACH(const String& domain, domains) {
						Domain::Ptr domainObj = Domain::GetByName(domain);

						if (!domainObj)
							BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid domain: " + domain));

						privs |= domainObj->GetPrivileges(endpoint->GetName());
					}
				} else {
					privs = INT_MAX;
				}

				Log(LogDebug, "cluster", "Privileges for object '" + object->GetName() + "' of type '" + object->GetType()->GetName() + "' for instance '" + endpoint->GetName() + "' are '" + Convert::ToString(privs) + "'");
				object->SetPrivileges(endpoint->GetName(), privs);
			}
		}
	}
}

/**
 * Stops the component.
 */
void ClusterListener::Stop(void)
{
	ObjectLock olock(this);
	CloseLogFile();
	RotateLogFile();
}

shared_ptr<SSL_CTX> ClusterListener::GetSSLContext(void) const
{
	return m_SSLContext;
}

/**
 * Creates a new JSON-RPC listener on the specified port.
 *
 * @param service The port to listen on.
 */
void ClusterListener::AddListener(const String& service)
{
	ObjectLock olock(this);

	shared_ptr<SSL_CTX> sslContext = m_SSLContext;

	if (!sslContext)
		BOOST_THROW_EXCEPTION(std::logic_error("SSL context is required for AddListener()"));

	std::ostringstream s;
	s << "Adding new listener: port " << service;
	Log(LogInformation, "cluster", s.str());

	TcpSocket::Ptr server = make_shared<TcpSocket>();
	server->Bind(service, AF_INET6);

	boost::thread thread(boost::bind(&ClusterListener::ListenerThreadProc, this, server));
	thread.detach();

	m_Servers.insert(server);
}

void ClusterListener::ListenerThreadProc(const Socket::Ptr& server)
{
	Utility::SetThreadName("Cluster Listener");

	server->Listen();

	for (;;) {
		Socket::Ptr client = server->Accept();

		Utility::QueueAsyncCallback(boost::bind(&ClusterListener::NewClientHandler, this, client, TlsRoleServer));
	}
}

/**
 * Creates a new JSON-RPC client and connects to the specified host and port.
 *
 * @param node The remote host.
 * @param service The remote port.
 */
void ClusterListener::AddConnection(const String& node, const String& service) {
	{
		ObjectLock olock(this);

		shared_ptr<SSL_CTX> sslContext = m_SSLContext;

		if (!sslContext)
			BOOST_THROW_EXCEPTION(std::logic_error("SSL context is required for AddConnection()"));
	}

	TcpSocket::Ptr client = make_shared<TcpSocket>();

	client->Connect(node, service);
	Utility::QueueAsyncCallback(boost::bind(&ClusterListener::NewClientHandler, this, client, TlsRoleClient));
}

void ClusterListener::AsyncRelayMessage(const Endpoint::Ptr& source, const Dictionary::Ptr& message, bool persistent)
{
	m_RelayQueue.Enqueue(boost::bind(&ClusterListener::RelayMessage, this, source, message, persistent));
}

void ClusterListener::PersistMessage(const Endpoint::Ptr& source, const Dictionary::Ptr& message)
{
	double ts = message->Get("ts");

	ASSERT(ts != 0);

	Dictionary::Ptr pmessage = make_shared<Dictionary>();
	pmessage->Set("timestamp", ts);

	if (source)
		pmessage->Set("source", source->GetName());

	pmessage->Set("message", JsonSerialize(message));
	pmessage->Set("security", message->Get("security"));

	ObjectLock olock(this);
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

void ClusterListener::RelayMessage(const Endpoint::Ptr& source, const Dictionary::Ptr& message, bool persistent)
{
	double ts = Utility::GetTime();
	message->Set("ts", ts);

	if (persistent)
		m_LogQueue.Enqueue(boost::bind(&ClusterListener::PersistMessage, this, source, message));

	Dictionary::Ptr security = message->Get("security");
	DynamicObject::Ptr secobj;
	int privs = 0;

	if (security) {
		String type = security->Get("type");
		DynamicType::Ptr dtype = DynamicType::GetByName(type);

		if (!dtype) {
			Log(LogWarning, "cluster", "Invalid type in security attribute: " + type);
			return;
		}

		String name = security->Get("name");
		secobj = dtype->GetObject(name);

		if (!secobj) {
			Log(LogWarning, "cluster", "Invalid object name in security attribute: " + name + " (of type '" + type + "')");
			return;
		}

		privs = security->Get("privs");
	}

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
		if (!persistent && !endpoint->IsConnected())
			continue;

		if (endpoint == source)
			continue;

		if (endpoint->GetName() == GetIdentity())
			continue;

		if (secobj && !secobj->HasPrivileges(endpoint->GetName(), privs)) {
			Log(LogDebug, "cluster", "Not sending message to endpoint '" + endpoint->GetName() + "': Insufficient privileges.");
			continue;
		}

		{
			ObjectLock olock(endpoint);

			if (!endpoint->GetSyncing())
				endpoint->SendMessage(message);
		}
	}
}

String ClusterListener::GetClusterDir(void) const
{
	return Application::GetLocalStateDir() + "/lib/icinga2/cluster/";
}

void ClusterListener::OpenLogFile(void)
{
	ASSERT(OwnsLock());

	String path = GetClusterDir() + "log/current";

	std::fstream *fp = new std::fstream(path.CStr(), std::fstream::out | std::ofstream::app);

	if (!fp->good()) {
		Log(LogWarning, "cluster", "Could not open spool file: " + path);
		return;
	}

	StdioStream::Ptr logStream = make_shared<StdioStream>(fp, true);
#ifdef HAVE_BIOZLIB
	m_LogFile = make_shared<ZlibStream>(logStream);
#else /* HAVE_BIOZLIB */
	m_LogFile = logStream;
#endif /* HAVE_BIOZLIB */
	m_LogMessageCount = 0;
	SetLogMessageTimestamp(0);
}

void ClusterListener::CloseLogFile(void)
{
	ASSERT(OwnsLock());

	if (!m_LogFile)
		return;

	m_LogFile->Close();
	m_LogFile.reset();

}

void ClusterListener::RotateLogFile(void)
{
	ASSERT(OwnsLock());

	double ts = GetLogMessageTimestamp();

	if (ts == 0)
		ts = Utility::GetTime();

	String oldpath = GetClusterDir() + "log/current";
	String newpath = GetClusterDir() + "log/" + Convert::ToString(static_cast<int>(ts) + 1);
	(void) rename(oldpath.CStr(), newpath.CStr());
}

void ClusterListener::LogGlobHandler(std::vector<int>& files, const String& file)
{
	String name = Utility::BaseName(file);

	int ts;

	try {
		ts = Convert::ToLong(name);
	} catch (const std::exception&) {
		return;
	}

	files.push_back(ts);
}

void ClusterListener::ReplayLog(const Endpoint::Ptr& endpoint, const Stream::Ptr& stream)
{
	CONTEXT("Replaying log for Endpoint '" + endpoint->GetName() + "'");

	int count = -1;
	double peer_ts = endpoint->GetLocalLogPosition();
	bool last_sync = false;

	ASSERT(!OwnsLock());

	for (;;) {
		ObjectLock olock(this);

		CloseLogFile();
		RotateLogFile();

		if (count == -1 || count > 50000) {
			OpenLogFile();
			olock.Unlock();
		} else {
			last_sync = true;
		}

		count = 0;

		std::vector<int> files;
		Utility::Glob(GetClusterDir() + "log/*", boost::bind(&ClusterListener::LogGlobHandler, boost::ref(files), _1), GlobFile);
		std::sort(files.begin(), files.end());

		BOOST_FOREACH(int ts, files) {
			String path = GetClusterDir() + "log/" + Convert::ToString(ts);

			if (ts < peer_ts)
				continue;

			Log(LogInformation, "cluster", "Replaying log: " + path);

			std::fstream *fp = new std::fstream(path.CStr(), std::fstream::in);
			StdioStream::Ptr logStream = make_shared<StdioStream>(fp, true);
#ifdef HAVE_BIOZLIB
			ZlibStream::Ptr lstream = make_shared<ZlibStream>(logStream);
#else /* HAVE_BIOZLIB */
			Stream::Ptr lstream = logStream;
#endif /* HAVE_BIOZLIB */

			String message;
			while (true) {
				Dictionary::Ptr pmessage;

				try {
					if (!NetString::ReadStringFromStream(lstream, &message))
						break;

					pmessage = JsonDeserialize(message);
				} catch (std::exception&) {
					Log(LogWarning, "cluster", "Unexpected end-of-file for cluster log: " + path);

					/* Log files may be incomplete or corrupted. This is perfectly OK. */
					break;
				}

				if (pmessage->Get("timestamp") < peer_ts)
					continue;

				if (pmessage->Get("source") == endpoint->GetName())
					continue;

				Dictionary::Ptr security = pmessage->Get("security");
				DynamicObject::Ptr secobj;
				int privs;

				if (security) {
					String type = security->Get("type");
					DynamicType::Ptr dtype = DynamicType::GetByName(type);

					if (!dtype) {
						Log(LogDebug, "cluster", "Invalid type in security attribute: " + type);
						continue;
					}

					String name = security->Get("name");
					secobj = dtype->GetObject(name);

					if (!secobj) {
						Log(LogDebug, "cluster", "Invalid object name in security attribute: " + name + " (of type '" + type + "')");
						continue;
					}

					privs = security->Get("privs");
				}

				if (secobj && !secobj->HasPrivileges(endpoint->GetName(), privs)) {
					Log(LogDebug, "cluster", "Not replaying message to endpoint '" + endpoint->GetName() + "': Insufficient privileges.");
					continue;
				}

				NetString::WriteStringToStream(stream, pmessage->Get("message"));
				count++;

				peer_ts = pmessage->Get("timestamp");
			}

			lstream->Close();
		}

		Log(LogInformation, "cluster", "Replayed " + Convert::ToString(count) + " messages.");

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

void ClusterListener::ConfigGlobHandler(const Dictionary::Ptr& config, const String& file, bool basename)
{
	CONTEXT("Creating config update for file '" + file + "'");

	Dictionary::Ptr elem = make_shared<Dictionary>();

	std::ifstream fp(file.CStr());
	if (!fp)
		return;

	String content((std::istreambuf_iterator<char>(fp)), std::istreambuf_iterator<char>());
	elem->Set("content", content);

	config->Set(basename ? Utility::BaseName(file) : file, elem);
}

/**
 * Processes a new client connection.
 *
 * @param client The new client.
 */
void ClusterListener::NewClientHandler(const Socket::Ptr& client, TlsRole role)
{
	CONTEXT("Handling new cluster client connection");

	NetworkStream::Ptr netStream = make_shared<NetworkStream>(client);

	TlsStream::Ptr tlsStream = make_shared<TlsStream>(netStream, role, m_SSLContext);
	tlsStream->Handshake();

	shared_ptr<X509> cert = tlsStream->GetPeerCertificate();
	String identity = GetCertificateCN(cert);

	Log(LogInformation, "cluster", "New client connection for identity '" + identity + "'");

	Endpoint::Ptr endpoint = Endpoint::GetByName(identity);

	if (!endpoint) {
		Log(LogInformation, "cluster", "Closing endpoint '" + identity + "': No configuration available.");
		tlsStream->Close();
		return;
	}

	{
		ObjectLock olock(endpoint);

		endpoint->SetSyncing(true);
		endpoint->SetSeen(Utility::GetTime());
		endpoint->SetClient(tlsStream);
	}

	Dictionary::Ptr config = make_shared<Dictionary>();
	Array::Ptr configFiles = endpoint->GetConfigFiles();

	if (configFiles) {
		ObjectLock olock(configFiles);
		BOOST_FOREACH(const String& pattern, configFiles) {
			Utility::Glob(pattern, boost::bind(&ClusterListener::ConfigGlobHandler, boost::cref(config), _1, false), GlobFile);
		}
	}

	Log(LogInformation, "cluster", "Sending " + Convert::ToString(static_cast<long>(config->GetLength())) + " config files to endpoint '" + endpoint->GetName() + "'.");

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("identity", GetIdentity());
	params->Set("config_files", config);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::Config");
	message->Set("params", params);

	NetString::WriteStringToStream(tlsStream, JsonSerialize(message));

	ReplayLog(endpoint, tlsStream);
}

void ClusterListener::ClusterTimerHandler(void)
{
	/* broadcast a heartbeat message */
	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("identity", GetIdentity());

	/* Eww. */
	Dictionary::Ptr features = make_shared<Dictionary>();
	features->Set("checker", SupportsChecks() ? 1 : 0);
	features->Set("notification", SupportsNotifications() ? 1 : 0);
	params->Set("features", features);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::HeartBeat");
	message->Set("params", params);

	Endpoint::GetByName(GetIdentity())->SetFeatures(features);

	AsyncRelayMessage(Endpoint::Ptr(), message, false);

	{
		ObjectLock olock(this);
		/* check if we've recently seen heartbeat messages from our peers */
		BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
			if (endpoint->GetSeen() > Utility::GetTime() - 60)
				continue;

			Stream::Ptr client = endpoint->GetClient();

			if (client) {
				Log(LogWarning, "cluster", "Closing connection for endpoint '" + endpoint->GetName() + "' due to inactivity.");
				client->Close();
				endpoint->SetClient(Stream::Ptr());
			}
		}
	}

	std::vector<int> files;
	Utility::Glob(GetClusterDir() + "log/*", boost::bind(&ClusterListener::LogGlobHandler, boost::ref(files), _1), GlobFile);
	std::sort(files.begin(), files.end());

	BOOST_FOREACH(int ts, files) {
		bool need = false;

		BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
			if (endpoint->GetName() == GetIdentity())
				continue;

			double position = endpoint->GetLocalLogPosition();

			if (position != 0 && ts > position) {
				need = true;
				break;
			}
		}

		if (!need) {
			String path = GetClusterDir() + "log/" + Convert::ToString(ts);
			Log(LogInformation, "cluster", "Removing old log file: " + path);
			(void) unlink(path.CStr());
		}
	}

	UpdateAuthority();

	Array::Ptr peers = GetPeers();

	if (peers) {
		ObjectLock olock(peers);
		BOOST_FOREACH(const String& peer, peers) {
			Endpoint::Ptr endpoint = Endpoint::GetByName(peer);

			if (!endpoint) {
				Log(LogWarning, "cluster", "Attempted to reconnect to endpoint '" + peer + "': No configuration found.");
				continue;
			}

			if (endpoint->IsConnected())
				continue;

			String host, port;
			host = endpoint->GetHost();
			port = endpoint->GetPort();

			if (host.IsEmpty() || port.IsEmpty()) {
				Log(LogWarning, "cluster", "Can't reconnect "
				    "to endpoint '" + endpoint->GetName() + "': No "
				    "host/port information.");
				continue;
			}

			try {
				Log(LogInformation, "cluster", "Attempting to reconnect to cluster endpoint '" + endpoint->GetName() + "' via '" + host + ":" + port + "'.");
				AddConnection(host, port);
			} catch (std::exception& ex) {
				std::ostringstream msgbuf;
				msgbuf << "Exception occured while reconnecting to endpoint '"
				       << endpoint->GetName() << "': " << DiagnosticInformation(ex);
				Log(LogWarning, "cluster", msgbuf.str());
			}
		}
	}
}

void ClusterListener::SetSecurityInfo(const Dictionary::Ptr& message, const DynamicObject::Ptr& object, int privs)
{
	ASSERT(object);

	Dictionary::Ptr security = make_shared<Dictionary>();
	security->Set("type", object->GetType()->GetName());
	security->Set("name", object->GetName());
	security->Set("privs", privs);

	message->Set("security", security);
}

void ClusterListener::CheckResultHandler(const Service::Ptr& service, const CheckResult::Ptr& cr, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("check_result", Serialize(cr));

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::CheckResult");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::NextCheckChangedHandler(const Service::Ptr& service, double nextCheck, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("next_check", nextCheck);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetNextCheck");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::NextNotificationChangedHandler(const Notification::Ptr& notification, double nextNotification, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("notification", notification->GetName());
	params->Set("next_notification", nextNotification);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetNextNotification");
	message->Set("params", params);

	SetSecurityInfo(message, notification->GetService(), DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::ForceNextCheckChangedHandler(const Service::Ptr& service, bool forced, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("forced", forced);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetForceNextCheck");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::ForceNextNotificationChangedHandler(const Service::Ptr& service, bool forced, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("forced", forced);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetForceNextNotification");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::EnableActiveChecksChangedHandler(const Service::Ptr& service, bool enabled, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("enabled", enabled);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetEnableActiveChecks");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::EnablePassiveChecksChangedHandler(const Service::Ptr& service, bool enabled, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("enabled", enabled);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetEnablePassiveChecks");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::EnableNotificationsChangedHandler(const Service::Ptr& service, bool enabled, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("enabled", enabled);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetEnableNotifications");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::EnableFlappingChangedHandler(const Service::Ptr& service, bool enabled, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("enabled", enabled);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetEnableFlapping");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::CommentAddedHandler(const Service::Ptr& service, const Comment::Ptr& comment, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("comment", Serialize(comment));

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::AddComment");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::CommentRemovedHandler(const Service::Ptr& service, const Comment::Ptr& comment, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("id", comment->GetId());

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::RemoveComment");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::DowntimeAddedHandler(const Service::Ptr& service, const Downtime::Ptr& downtime, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("downtime", Serialize(downtime));

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::AddDowntime");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::DowntimeRemovedHandler(const Service::Ptr& service, const Downtime::Ptr& downtime, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("id", downtime->GetId());

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::RemoveDowntime");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::AcknowledgementSetHandler(const Service::Ptr& service, const String& author, const String& comment, AcknowledgementType type, double expiry, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("author", author);
	params->Set("comment", comment);
	params->Set("type", type);
	params->Set("expiry", expiry);

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetAcknowledgement");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::AcknowledgementClearedHandler(const Service::Ptr& service, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = make_shared<Dictionary>();
	params->Set("service", service->GetName());

	Dictionary::Ptr message = make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::ClearAcknowledgement");
	message->Set("params", params);

	SetSecurityInfo(message, service, DomainPrivRead);

	AsyncRelayMessage(Endpoint::Ptr(), message, true);
}

void ClusterListener::AsyncMessageHandler(const Endpoint::Ptr& sender, const Dictionary::Ptr& message)
{
	m_MessageQueue.Enqueue(boost::bind(&ClusterListener::MessageHandler, this, sender, message));
}

void ClusterListener::MessageExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "cluster", "Exception while processing cluster message: " + DiagnosticInformation(exp));
}

void ClusterListener::MessageHandler(const Endpoint::Ptr& sender, const Dictionary::Ptr& message)
{
	CONTEXT("Processing cluster message of type '" + message->Get("method") + "'");

	sender->SetSeen(Utility::GetTime());

	if (message->Contains("ts")) {
		double ts = message->Get("ts");

		/* ignore old messages */
		if (ts < sender->GetRemoteLogPosition())
			return;

		if (sender->GetRemoteLogPosition() + 10 < ts) {
			Dictionary::Ptr lparams = make_shared<Dictionary>();
			lparams->Set("log_position", message->Get("ts"));

			Dictionary::Ptr lmessage = make_shared<Dictionary>();
			lmessage->Set("jsonrpc", "2.0");
			lmessage->Set("method", "cluster::SetLogPosition");
			lmessage->Set("params", lparams);

			sender->SendMessage(lmessage);

			sender->SetRemoteLogPosition(message->Get("ts"));

			Log(LogInformation, "cluster", "Acknowledging log position for identity '" + sender->GetName() + "': " + Utility::FormatDateTime("%Y/%m/%d %H:%M:%S", message->Get("ts")));
		}
	}

	Dictionary::Ptr params = message->Get("params");

	if (message->Get("method") == "cluster::HeartBeat") {
		if (!params)
			return;

		String identity = params->Get("identity");

		Endpoint::Ptr endpoint = Endpoint::GetByName(identity);

		if (endpoint) {
			endpoint->SetSeen(Utility::GetTime());
			endpoint->SetFeatures(params->Get("features"));
		}

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::CheckResult") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::CheckResult message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		CheckResult::Ptr cr = Deserialize(params->Get("check_result"), true);

		if (!cr)
			return;

		service->ProcessCheckResult(cr, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::SetNextCheck") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::SetNextCheck message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		double nextCheck = params->Get("next_check");

		service->SetNextCheck(nextCheck, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::SetForceNextCheck") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::SetForceNextCheck message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		bool forced = params->Get("forced");

		service->SetForceNextCheck(forced, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::SetForceNextNotification") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::SetForceNextNotification message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		bool forced = params->Get("forced");

		service->SetForceNextNotification(forced, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::SetEnableActiveChecks") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::SetEnableActiveChecks message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		bool enabled = params->Get("enabled");

		service->SetEnableActiveChecks(enabled, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::SetEnablePassiveChecks") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::SetEnablePassiveChecks message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		bool enabled = params->Get("enabled");

		service->SetEnablePassiveChecks(enabled, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::SetEnableNotifications") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::SetEnableNotifications message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		bool enabled = params->Get("enabled");

		service->SetEnableNotifications(enabled, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::SetEnableFlapping") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::SetEnableFlapping message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		bool enabled = params->Get("enabled");

		service->SetEnableFlapping(enabled, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::SetNextNotification") {
		if (!params)
			return;

		String nfc = params->Get("notification");

		Notification::Ptr notification = Notification::GetByName(nfc);

		if (!notification)
			return;

		Service::Ptr service = notification->GetService();

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::SetNextNotification message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		bool nextNotification = params->Get("next_notification");

		notification->SetNextNotification(nextNotification, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::AddComment") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::AddComment message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		Comment::Ptr comment = Deserialize(params->Get("comment"), true);

		service->AddComment(comment->GetEntryType(), comment->GetAuthor(),
		    comment->GetText(), comment->GetExpireTime(), comment->GetId(), sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::RemoveComment") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::RemoveComment message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		String id = params->Get("id");

		service->RemoveComment(id, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::AddDowntime") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::AddDowntime message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		Downtime::Ptr downtime = Deserialize(params->Get("downtime"), true);

		service->AddDowntime(downtime->GetAuthor(), downtime->GetComment(),
		    downtime->GetStartTime(), downtime->GetEndTime(),
		    downtime->GetFixed(), downtime->GetTriggeredBy(),
		    downtime->GetDuration(), downtime->GetScheduledBy(),
		    downtime->GetId(), sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::RemoveDowntime") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::RemoveDowntime message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		String id = params->Get("id");

		service->RemoveDowntime(id, false, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::SetAcknowledgement") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::SetAcknowledgement message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		String author = params->Get("author");
		String comment = params->Get("comment");
		int type = params->Get("type");
		double expiry = params->Get("expiry");

		service->AcknowledgeProblem(author, comment, static_cast<AcknowledgementType>(type), expiry, sender->GetName());

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::ClearAcknowledgement") {
		if (!params)
			return;

		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		if (!service->HasPrivileges(sender->GetName(), DomainPrivCommand)) {
			Log(LogDebug, "cluster", "Not accepting cluster::ClearAcknowledgement message from endpoint '" + sender->GetName() + "' for service '" + service->GetName() + "': Insufficient privileges.");
			return;
		}

		{
			ObjectLock olock(service);
			service->ClearAcknowledgement(sender->GetName());
		}

		AsyncRelayMessage(sender, message, true);
	} else if (message->Get("method") == "cluster::SetLogPosition") {
		if (!params)
			return;

		sender->SetLocalLogPosition(params->Get("log_position"));
	} else if (message->Get("method") == "cluster::Config") {
		if (!params)
			return;

		Dictionary::Ptr remoteConfig = params->Get("config_files");
		
		if (!remoteConfig)
			return;

		Endpoint::Ptr self = Endpoint::GetByName(GetIdentity());

		Array::Ptr acceptConfig = self->GetAcceptConfig();

		bool accept = false;

		if (acceptConfig) {
			ObjectLock olock(acceptConfig);
			BOOST_FOREACH(const String& pattern, acceptConfig) {
				if (pattern == sender->GetName()) {
					accept = true;
					break;
				}
			}
		}

		String identity = params->Get("identity");

		if (!accept) {
			Log(LogWarning, "cluster", "Ignoring config update from endpoint '" + sender->GetName() + "' for identity '" + identity + "'.");
			return;
		}

		Log(LogInformation, "cluster", "Processing config update for identity '" + identity + "'.");

		String dir = GetClusterDir() + "config/" + SHA256(identity);
#ifndef _WIN32
		if (mkdir(dir.CStr(), 0700) < 0 && errno != EEXIST) {
#else /*_ WIN32 */
		if (mkdir(dir.CStr()) < 0 && errno != EEXIST) {
#endif /* _WIN32 */
			BOOST_THROW_EXCEPTION(posix_error()
				<< boost::errinfo_api_function("localtime")
				<< boost::errinfo_errno(errno));
		}

		Dictionary::Ptr localConfig = make_shared<Dictionary>();
		Utility::Glob(dir + "/*", boost::bind(&ClusterListener::ConfigGlobHandler, boost::cref(localConfig), _1, true), GlobFile);

		bool configChange = false;

		/* figure out whether config files were removed */
		if (localConfig->GetLength() != remoteConfig->GetLength())
			configChange = true;

		ObjectLock olock(remoteConfig);
		BOOST_FOREACH(const Dictionary::Pair& kv, remoteConfig) {
			Dictionary::Ptr remoteFile = kv.second;
			bool writeFile = false;
			String hash = SHA256(kv.first);
			String path = dir + "/" + hash;
			
			if (!localConfig->Contains(hash))
				writeFile = true;
			else {
				Dictionary::Ptr localFile = localConfig->Get(hash);

				String localContent = localFile->Get("content");
				String remoteContent = remoteFile->Get("content");

				if (localContent != remoteContent)
					writeFile = true;
			}

			if (writeFile) {
				configChange = true;

				Log(LogInformation, "cluster", "Updating configuration file: " + path);

				std::ofstream fp(path.CStr(), std::ofstream::out | std::ostream::trunc);
				fp << remoteFile->Get("content");
				fp.close();
			}

			localConfig->Remove(hash);
		}
		olock.Unlock();

		ObjectLock olock2(localConfig);
		BOOST_FOREACH(const Dictionary::Pair& kv, localConfig) {
			String path = dir + "/" + kv.first;
			Log(LogInformation, "cluster", "Removing obsolete config file: " + path);
			(void) unlink(path.CStr());
			configChange = true;
		}
		olock2.Unlock();

		if (configChange) {
			Log(LogInformation, "cluster", "Restarting after configuration change.");
			Application::RequestRestart();
		}

		AsyncRelayMessage(sender, message, true);
	}
}

bool ClusterListener::IsAuthority(const DynamicObject::Ptr& object, const String& type)
{
	Array::Ptr authorities = object->GetAuthorities();
	std::vector<String> endpoints;

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
		bool match = false;

		if ((!endpoint->IsConnected() && endpoint->GetName() != GetIdentity()) || !endpoint->HasFeature(type))
			continue;

		if (authorities) {
			ObjectLock olock(authorities);
			BOOST_FOREACH(const String& authority, authorities) {
				if (authority == endpoint->GetName()) {
					match = true;

					break;
				}
			}
		} else {
			match = true;
		}

		if (match)
			endpoints.push_back(endpoint->GetName());
	}

	if (endpoints.empty())
		return false;

	std::sort(endpoints.begin(), endpoints.end());

	String key = object->GetType()->GetName() + "\t" + object->GetName();
	unsigned long hash = Utility::SDBM(key);
	unsigned long index = hash % endpoints.size();

//	Log(LogDebug, "cluster", "Authority for object '" + object->GetName() + "' of type '" + object->GetType()->GetName() + "' is '" + endpoints[index] + "'.");

	return (endpoints[index] == GetIdentity());
}

void ClusterListener::UpdateAuthority(void)
{
	Log(LogDebug, "cluster", "Updating authority for objects.");

	int checker_count = 0, notifications_count = 0;

	BOOST_FOREACH(const DynamicType::Ptr& type, DynamicType::GetTypes()) {
		BOOST_FOREACH(const DynamicObject::Ptr& object, type->GetObjects()) {
			bool checkerAuthority = IsAuthority(object, "checker");

			if (checkerAuthority)
				checker_count++;

			object->SetAuthority("checker", checkerAuthority);

			bool notificationAuthority = IsAuthority(object, "notifications");

			if (notificationAuthority)
				notifications_count++;

			object->SetAuthority("notifications", notificationAuthority);
		}
	}

	Log(LogDebug, "cluster", "Cluster authority: " + Convert::ToString(checker_count) + "x checker, " + Convert::ToString(notifications_count) + "x notifications");
}

bool ClusterListener::SupportsChecks(void)
{
	DynamicType::Ptr type = DynamicType::GetByName("CheckerComponent");

	if (!type)
		return false;

	return !type->GetObjects().empty() && IcingaApplication::GetInstance()->GetEnableChecks();
}

bool ClusterListener::SupportsNotifications(void)
{
	DynamicType::Ptr type = DynamicType::GetByName("NotificationComponent");

	if (!type)
		return false;

	return !type->GetObjects().empty() && IcingaApplication::GetInstance()->GetEnableNotifications();
}
