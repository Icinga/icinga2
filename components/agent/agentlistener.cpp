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

#include "agent/agentlistener.h"
#include "remote/jsonrpc.h"
#include "icinga/icingaapplication.h"
#include "base/netstring.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/objectlock.h"
#include "base/networkstream.h"
#include "base/application.h"
#include "base/context.h"
#include <fstream>

using namespace icinga;

REGISTER_TYPE(AgentListener);

/**
 * Starts the component.
 */
void AgentListener::Start(void)
{
	DynamicObject::Start();

	m_Results = make_shared<Dictionary>();

	/* set up SSL context */
	shared_ptr<X509> cert = GetX509Certificate(GetCertPath());
	SetIdentity(GetCertificateCN(cert));
	Log(LogInformation, "agent", "My identity: " + GetIdentity());

	m_SSLContext = MakeSSLContext(GetCertPath(), GetKeyPath(), GetCaPath());

	if (!GetCrlPath().IsEmpty())
		AddCRLToSSLContext(m_SSLContext, GetCrlPath());

	/* create the primary JSON-RPC listener */
	if (!GetBindPort().IsEmpty())
		AddListener(GetBindPort());

	m_AgentTimer = make_shared<Timer>();
	m_AgentTimer->OnTimerExpired.connect(boost::bind(&AgentListener::AgentTimerHandler, this));
	m_AgentTimer->SetInterval(GetUpstreamInterval());
	m_AgentTimer->Start();
	m_AgentTimer->Reschedule(0);
}

shared_ptr<SSL_CTX> AgentListener::GetSSLContext(void) const
{
	return m_SSLContext;
}

String AgentListener::GetInventoryDir(void)
{
	return Application::GetLocalStateDir() + "/lib/icinga2/agent/inventory/";
}

/**
 * Creates a new JSON-RPC listener on the specified port.
 *
 * @param service The port to listen on.
 */
void AgentListener::AddListener(const String& service)
{
	ObjectLock olock(this);

	shared_ptr<SSL_CTX> sslContext = m_SSLContext;

	if (!sslContext)
		BOOST_THROW_EXCEPTION(std::logic_error("SSL context is required for AddListener()"));

	std::ostringstream s;
	s << "Adding new listener: port " << service;
	Log(LogInformation, "agent", s.str());

	TcpSocket::Ptr server = make_shared<TcpSocket>();
	server->Bind(service, AF_INET6);

	boost::thread thread(boost::bind(&AgentListener::ListenerThreadProc, this, server));
	thread.detach();

	m_Servers.insert(server);
}

void AgentListener::ListenerThreadProc(const Socket::Ptr& server)
{
	Utility::SetThreadName("Cluster Listener");

	server->Listen();

	for (;;) {
		Socket::Ptr client = server->Accept();

		Utility::QueueAsyncCallback(boost::bind(&AgentListener::NewClientHandler, this, client, TlsRoleServer));
	}
}

/**
 * Creates a new JSON-RPC client and connects to the specified host and port.
 *
 * @param node The remote host.
 * @param service The remote port.
 */
void AgentListener::AddConnection(const String& node, const String& service) {
	{
		ObjectLock olock(this);

		shared_ptr<SSL_CTX> sslContext = m_SSLContext;

		if (!sslContext)
			BOOST_THROW_EXCEPTION(std::logic_error("SSL context is required for AddConnection()"));
	}

	TcpSocket::Ptr client = make_shared<TcpSocket>();

	client->Connect(node, service);
	Utility::QueueAsyncCallback(boost::bind(&AgentListener::NewClientHandler, this, client, TlsRoleClient));
}

/**
 * Processes a new client connection.
 *
 * @param client The new client.
 */
void AgentListener::NewClientHandler(const Socket::Ptr& client, TlsRole role)
{
	CONTEXT("Handling new agent client connection");

	TlsStream::Ptr tlsStream;

	{
		ObjectLock olock(this);
		tlsStream = make_shared<TlsStream>(client, role, m_SSLContext);
	}

	tlsStream->Handshake();

	shared_ptr<X509> cert = tlsStream->GetPeerCertificate();
	String identity = GetCertificateCN(cert);

	Log(LogInformation, "agent", "New client connection for identity '" + identity + "'");

	if (identity != GetUpstreamName()) {
		Dictionary::Ptr request = make_shared<Dictionary>();
		request->Set("method", "get_crs");
		JsonRpc::SendMessage(tlsStream, request);
	}

	try {
		Dictionary::Ptr message = JsonRpc::ReadMessage(tlsStream);
		MessageHandler(tlsStream, identity, message);
	} catch (const std::exception& ex) {
		Log(LogWarning, "agent", "Error while reading JSON-RPC message for agent '" + identity + "': " + DiagnosticInformation(ex));
	}

	tlsStream->Close();
}

void AgentListener::MessageHandler(const TlsStream::Ptr& sender, const String& identity, const Dictionary::Ptr& message)
{
	CONTEXT("Processing agent message of type '" + message->Get("method") + "'");

	String method = message->Get("method");

	if (identity == GetUpstreamName()) {
		if (method == "get_crs") {
			Dictionary::Ptr hosts = make_shared<Dictionary>();

			BOOST_FOREACH(const Host::Ptr& host, DynamicType::GetObjects<Host>()) {
				Dictionary::Ptr hostInfo = make_shared<Dictionary>();

				hostInfo->Set("cr", Serialize(host->GetLastCheckResult()));

				Dictionary::Ptr services = make_shared<Dictionary>();

				BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
					Dictionary::Ptr serviceInfo = make_shared<Dictionary>();
					serviceInfo->Set("cr", Serialize(service->GetLastCheckResult()));
					services->Set(service->GetShortName(), serviceInfo);
				}

				hostInfo->Set("services", services);

				hosts->Set(host->GetName(), hostInfo);
			}

			Dictionary::Ptr params = make_shared<Dictionary>();
			params->Set("hosts", hosts);

			Dictionary::Ptr request = make_shared<Dictionary>();
			request->Set("method", "push_crs");
			request->Set("params", params);

			JsonRpc::SendMessage(sender, request);
		}
	}

	if (method == "push_crs") {
		Value paramsv = message->Get("params");

		if (paramsv.IsEmpty() || !paramsv.IsObjectType<Dictionary>())
			return;

		Dictionary::Ptr params = paramsv;

		params->Set("seen", Utility::GetTime());

		Dictionary::Ptr inventoryDescr = make_shared<Dictionary>();
		inventoryDescr->Set("identity", identity);
		inventoryDescr->Set("params", params);

		String inventoryFile = GetInventoryDir() + SHA256(identity);
		String inventoryTempFile = inventoryFile + ".tmp";

		std::ofstream fp(inventoryTempFile.CStr(), std::ofstream::out | std::ostream::trunc);
		fp << JsonSerialize(inventoryDescr);
		fp.close();

#ifdef _WIN32
		_unlink(inventoryFile.CStr());
#endif /* _WIN32 */

		if (rename(inventoryTempFile.CStr(), inventoryFile.CStr()) < 0) {
			BOOST_THROW_EXCEPTION(posix_error()
			    << boost::errinfo_api_function("rename")
			    << boost::errinfo_errno(errno)
			    << boost::errinfo_file_name(inventoryTempFile));
		}

		m_Results->Set(identity, params);
	}
}

double  AgentListener::GetAgentSeen(const String& agentIdentity)
{
	Dictionary::Ptr agentparams = m_Results->Get(agentIdentity);

	if (!agentparams)
		return 0;

	return agentparams->Get("seen");
}

CheckResult::Ptr AgentListener::GetCheckResult(const String& agentIdentity, const String& hostName, const String& serviceName)
{
	Dictionary::Ptr agentparams = m_Results->Get(agentIdentity);

	if (!agentparams)
		return CheckResult::Ptr();

	Value hostsv = agentparams->Get("hosts");

	if (hostsv.IsEmpty() || !hostsv.IsObjectType<Dictionary>())
		return CheckResult::Ptr();

	Dictionary::Ptr hosts = hostsv;

	Value hostv = hosts->Get(hostName);

	if (hostv.IsEmpty() || !hostv.IsObjectType<Dictionary>())
		return CheckResult::Ptr();

	Dictionary::Ptr host = hostv;

	if (serviceName.IsEmpty()) {
		Value hostcrv = Deserialize(host->Get("cr"));

		if (hostcrv.IsEmpty() || !hostcrv.IsObjectType<CheckResult>())
			return CheckResult::Ptr();

		return hostcrv;
	} else {
		Value servicesv = host->Get("services");

		if (servicesv.IsEmpty() || !servicesv.IsObjectType<Dictionary>())
			return CheckResult::Ptr();

		Dictionary::Ptr services = servicesv;

		Value servicev = services->Get(serviceName);

		if (servicev.IsEmpty() || !servicev.IsObjectType<Dictionary>())
			return CheckResult::Ptr();

		Dictionary::Ptr service = servicev;

		Value servicecrv = Deserialize(service->Get("cr"));

		if (servicecrv.IsEmpty() || !servicecrv.IsObjectType<CheckResult>())
			return CheckResult::Ptr();

		return servicecrv;
	}
}

void AgentListener::AgentTimerHandler(void)
{
	String host = GetUpstreamHost();
	String port = GetUpstreamPort();

	if (host.IsEmpty() || port.IsEmpty())
		return;

	AddConnection(host, port);
}
