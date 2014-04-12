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

using namespace icinga;

REGISTER_TYPE(AgentListener);

/**
 * Starts the component.
 */
void AgentListener::Start(void)
{
	DynamicObject::Start();

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
}

shared_ptr<SSL_CTX> AgentListener::GetSSLContext(void) const
{
	return m_SSLContext;
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

	NetworkStream::Ptr netStream = make_shared<NetworkStream>(client);

	TlsStream::Ptr tlsStream;

	{
		ObjectLock olock(this);
		tlsStream = make_shared<TlsStream>(netStream, role, m_SSLContext);
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
			Dictionary::Ptr services = make_shared<Dictionary>();

			Host::Ptr host = Host::GetByName("localhost");
			
			if (!host)
				Log(LogWarning, "agent", "Agent doesn't have any services for 'localhost'.");
			else {
				BOOST_FOREACH(const Service::Ptr& service, host->GetServices()) {
					services->Set(service->GetShortName(), Serialize(service->GetLastCheckResult()));
				}
			}
			
			Dictionary::Ptr params = make_shared<Dictionary>();
			params->Set("services", services);
			params->Set("host", Serialize(host->GetLastCheckResult()));
			
			Dictionary::Ptr request = make_shared<Dictionary>();
			request->Set("method", "push_crs");
			request->Set("params", params);
			
			JsonRpc::SendMessage(sender, request);
		}
	}
	
	if (method == "push_crs") {
		Host::Ptr host = Host::GetByName(identity);
		
		if (!host) {
			Log(LogWarning, "agent", "Ignoring check results for host '" + identity + "'.");
			return;
		}
		
		Dictionary::Ptr params = message->Get("params");
		
		if (!params)
			return;

		Value hostcr = Deserialize(params->Get("host"), true);
		
		if (!hostcr.IsObjectType<CheckResult>()) {
			Log(LogWarning, "agent", "Ignoring invalid check result for host '" + identity + "'.");
		} else {
			CheckResult::Ptr cr = hostcr;
			host->ProcessCheckResult(cr);
		}

		Dictionary::Ptr services = params->Get("services");
		
		if (!services)
			return;
		
		Dictionary::Pair kv;
		
		BOOST_FOREACH(kv, services) {
			Service::Ptr service = host->GetServiceByShortName(kv.first);
			
			if (!service) {
				Log(LogWarning, "agent", "Ignoring check result for service '" + kv.first + "' on host '" + identity + "'.");
				continue;
			}
			
			Value servicecr = Deserialize(kv.second, true);
			
			if (!servicecr.IsObjectType<CheckResult>()) {
				Log(LogWarning, "agent", "Ignoring invalid check result for service '" + kv.first + "' on host '" + identity + "'.");
				continue;
			}
			
			CheckResult::Ptr cr = servicecr;
			service->ProcessCheckResult(cr);
		}
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
