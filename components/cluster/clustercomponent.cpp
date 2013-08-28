/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "cluster/clustercomponent.h"
#include "cluster/endpoint.h"
#include "base/dynamictype.h"
#include "base/logger_fwd.h"
#include "base/objectlock.h"
#include "base/networkstream.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

REGISTER_TYPE(ClusterComponent);

/**
 * Starts the component.
 */
void ClusterComponent::Start(void)
{
	DynamicObject::Start();

	/* set up SSL context */
	shared_ptr<X509> cert = GetX509Certificate(GetCertificateFile());
	m_Identity = GetCertificateCN(cert);
	Log(LogInformation, "cluster", "My identity: " + m_Identity);

	m_SSLContext = MakeSSLContext(GetCertificateFile(), GetCertificateFile(), GetCAFile());

	/* create the primary JSON-RPC listener */
	if (!GetBindPort().IsEmpty())
		AddListener(GetBindPort());

	m_ReconnectTimer = boost::make_shared<Timer>();
	m_ReconnectTimer->OnTimerExpired.connect(boost::bind(&ClusterComponent::ReconnectTimerHandler, this));
	m_ReconnectTimer->SetInterval(5);
	m_ReconnectTimer->Start();

	Service::OnNewCheckResult.connect(bind(&ClusterComponent::CheckResultHandler, this, _1, _2, _3));
	Service::OnNextCheckChanged.connect(bind(&ClusterComponent::NextCheckChangedHandler, this, _1, _2, _3));
	Service::OnForceNextCheckChanged.connect(bind(&ClusterComponent::ForceNextCheckChangedHandler, this, _1, _2, _3));
	Service::OnEnableActiveChecksChanged.connect(bind(&ClusterComponent::EnableActiveChecksChangedHandler, this, _1, _2, _3));
	Service::OnEnablePassiveChecksChanged.connect(bind(&ClusterComponent::EnablePassiveChecksChangedHandler, this, _1, _2, _3));

	Endpoint::OnMessageReceived.connect(bind(&ClusterComponent::MessageHandler, this, _1, _2));
}

/**
 * Stops the component.
 */
void ClusterComponent::Stop(void)
{
	/* Nothing to do here. */
}

String ClusterComponent::GetCertificateFile(void) const
{
	ObjectLock olock(this);

	return m_CertPath;
}

String ClusterComponent::GetCAFile(void) const
{
	ObjectLock olock(this);

	return m_CAPath;
}

String ClusterComponent::GetBindHost(void) const
{
	ObjectLock olock(this);

	return m_BindHost;
}

String ClusterComponent::GetBindPort(void) const
{
	ObjectLock olock(this);

	return m_BindPort;
}

Array::Ptr ClusterComponent::GetPeers(void) const
{
	ObjectLock olock(this);

	return m_Peers;
}

shared_ptr<SSL_CTX> ClusterComponent::GetSSLContext(void) const
{
	ObjectLock olock(this);

	return m_SSLContext;
}

String ClusterComponent::GetIdentity(void) const
{
	ObjectLock olock(this);

	return m_Identity;
}

/**
 * Creates a new JSON-RPC listener on the specified port.
 *
 * @param service The port to listen on.
 */
void ClusterComponent::AddListener(const String& service)
{
	ObjectLock olock(this);

	shared_ptr<SSL_CTX> sslContext = m_SSLContext;

	if (!sslContext)
		BOOST_THROW_EXCEPTION(std::logic_error("SSL context is required for AddListener()"));

	std::ostringstream s;
	s << "Adding new listener: port " << service;
	Log(LogInformation, "cluster", s.str());

	TcpSocket::Ptr server = boost::make_shared<TcpSocket>();
	server->Bind(service, AF_INET6);

	boost::thread thread(boost::bind(&ClusterComponent::ListenerThreadProc, this, server));
	thread.detach();

	m_Servers.insert(server);
}

void ClusterComponent::ListenerThreadProc(const Socket::Ptr& server)
{
	server->Listen();

	for (;;) {
		Socket::Ptr client = server->Accept();

		try {
			NewClientHandler(client, TlsRoleServer);
		} catch (const std::exception& ex) {
			std::stringstream message;
			message << "Error for new JSON-RPC socket: " << boost::diagnostic_information(ex);
			Log(LogInformation, "cluster", message.str());
		}
	}
}

/**
 * Creates a new JSON-RPC client and connects to the specified host and port.
 *
 * @param node The remote host.
 * @param service The remote port.
 */
void ClusterComponent::AddConnection(const String& node, const String& service) {
	{
		ObjectLock olock(this);

		shared_ptr<SSL_CTX> sslContext = m_SSLContext;

		if (!sslContext)
			BOOST_THROW_EXCEPTION(std::logic_error("SSL context is required for AddConnection()"));
	}

	TcpSocket::Ptr client = boost::make_shared<TcpSocket>();

	try {
		client->Connect(node, service);
		NewClientHandler(client, TlsRoleClient);
	} catch (const std::exception& ex) {
		Log(LogInformation, "cluster", "Could not connect to " + node + ":" + service + ": " + ex.what());
	}
}

/**
 * Processes a new client connection.
 *
 * @param client The new client.
 */
void ClusterComponent::NewClientHandler(const Socket::Ptr& client, TlsRole role)
{
	NetworkStream::Ptr netStream = boost::make_shared<NetworkStream>(client);

	TlsStream::Ptr tlsStream = boost::make_shared<TlsStream>(netStream, role, m_SSLContext);
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

	endpoint->SetClient(tlsStream);
}

void ClusterComponent::ReconnectTimerHandler(void)
{
	Array::Ptr peers = GetPeers();

	if (!peers)
		return;

	ObjectLock olock(peers);
	BOOST_FOREACH(const String& peer, peers) {
		Endpoint::Ptr endpoint = Endpoint::GetByName(peer);

		if (!endpoint)
			continue;

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

		Log(LogInformation, "cluster", "Attempting to reconnect to cluster endpoint '" + endpoint->GetName() + "' via '" + host + ":" + port + "'.");
		AddConnection(host, port);
	}
}

void ClusterComponent::CheckResultHandler(const Service::Ptr& service, const Dictionary::Ptr& cr, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = boost::make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("check_result", cr);

	Dictionary::Ptr message = boost::make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::CheckResult");
	message->Set("params", params);

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
		endpoint->SendMessage(message);
	}
}

void ClusterComponent::NextCheckChangedHandler(const Service::Ptr& service, double nextCheck, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = boost::make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("next_check", nextCheck);

	Dictionary::Ptr message = boost::make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetNextCheck");
	message->Set("params", params);

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
		endpoint->SendMessage(message);
	}
}

void ClusterComponent::ForceNextCheckChangedHandler(const Service::Ptr& service, bool forced, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = boost::make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("forced", forced);

	Dictionary::Ptr message = boost::make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetForceNextCheck");
	message->Set("params", params);

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
		endpoint->SendMessage(message);
	}
}

void ClusterComponent::EnableActiveChecksChangedHandler(const Service::Ptr& service, bool enabled, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = boost::make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("enabled", enabled);

	Dictionary::Ptr message = boost::make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetEnableActiveChecks");
	message->Set("params", params);

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
		endpoint->SendMessage(message);
	}
}

void ClusterComponent::EnablePassiveChecksChangedHandler(const Service::Ptr& service, bool enabled, const String& authority)
{
	if (!authority.IsEmpty() && authority != GetIdentity())
		return;

	Dictionary::Ptr params = boost::make_shared<Dictionary>();
	params->Set("service", service->GetName());
	params->Set("enabled", enabled);

	Dictionary::Ptr message = boost::make_shared<Dictionary>();
	message->Set("jsonrpc", "2.0");
	message->Set("method", "cluster::SetEnablePassiveChecks");
	message->Set("params", params);

	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
		endpoint->SendMessage(message);
	}
}

void ClusterComponent::MessageHandler(const Endpoint::Ptr& sender, const Dictionary::Ptr& message)
{
	BOOST_FOREACH(const Endpoint::Ptr& endpoint, DynamicType::GetObjects<Endpoint>()) {
		if (sender != endpoint)
			endpoint->SendMessage(message);
	}

	Dictionary::Ptr params = message->Get("params");

	if (!params)
		return;

	if (message->Get("method") == "cluster::CheckResult") {
		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		Dictionary::Ptr cr = params->Get("check_result");

		if (!cr)
			return;

		service->ProcessCheckResult(cr, sender->GetName());
	} else if (message->Get("method") == "cluster::SetNextCheck") {
		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		double nextCheck = params->Get("next_check");

		service->SetNextCheck(nextCheck, sender->GetName());
	} else if (message->Get("method") == "cluster::SetForceNextCheck") {
		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		bool forced = params->Get("forced");

		service->SetForceNextCheck(forced, sender->GetName());
	} else if (message->Get("method") == "cluster::SetEnableActiveChecks") {
		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		bool enabled = params->Get("enabled");

		service->SetEnableActiveChecks(enabled, sender->GetName());
	} else if (message->Get("method") == "cluster::SetEnablePassiveChecks") {
		String svc = params->Get("service");

		Service::Ptr service = Service::GetByName(svc);

		if (!service)
			return;

		bool enabled = params->Get("enabled");

		service->SetEnablePassiveChecks(enabled, sender->GetName());
	}
}

void ClusterComponent::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("cert_path", m_CertPath);
		bag->Set("ca_path", m_CAPath);
		bag->Set("bind_host", m_BindHost);
		bag->Set("bind_port", m_BindPort);
		bag->Set("peers", m_Peers);
	}
}

void ClusterComponent::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_CertPath = bag->Get("cert_path");
		m_CAPath = bag->Get("ca_path");
		m_BindHost = bag->Get("bind_host");
		m_BindPort = bag->Get("bind_port");
		m_Peers = bag->Get("peers");
	}
}
