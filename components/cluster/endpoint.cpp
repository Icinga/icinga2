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

#include "cluster/endpoint.h"
#include "cluster/jsonrpc.h"
#include "base/application.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/utility.h"
#include "base/logger_fwd.h"
#include "config/configitembuilder.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;

REGISTER_TYPE(Endpoint);

boost::signals2::signal<void (const Endpoint::Ptr&)> Endpoint::OnConnected;
boost::signals2::signal<void (const Endpoint::Ptr&, const Dictionary::Ptr&)> Endpoint::OnMessageReceived;

Endpoint::Endpoint(void)
	: m_Syncing(false)
{ }

/**
 * Checks whether this endpoint is connected.
 *
 * @returns true if the endpoint is connected, false otherwise.
 */
bool Endpoint::IsConnected(void) const
{
	return GetClient();
}

Stream::Ptr Endpoint::GetClient(void) const
{
	return m_Client;
}

void Endpoint::SetClient(const Stream::Ptr& client)
{
	if (m_Client)
		m_Client->Close();

	m_Client = client;

	if (client) {
		boost::thread thread(boost::bind(&Endpoint::MessageThreadProc, this, client));
		thread.detach();

		OnConnected(GetSelf());
	}
}

void Endpoint::SendMessage(const Dictionary::Ptr& message)
{
	Stream::Ptr client = GetClient();

	if (!client)
		return;

	try {
		JsonRpc::SendMessage(client, message);
	} catch (const std::exception& ex) {
		std::ostringstream msgbuf;
		msgbuf << "Error while sending JSON-RPC message for endpoint '" << GetName() << "': " << boost::diagnostic_information(ex);
		Log(LogWarning, "cluster", msgbuf.str());

		m_Client.reset();
	}
}

void Endpoint::MessageThreadProc(const Stream::Ptr& stream)
{
	Utility::SetThreadName("EndpointMsg");

	for (;;) {
		Dictionary::Ptr message;

		try {
			message = JsonRpc::ReadMessage(stream);
		} catch (const std::exception& ex) {
			Log(LogWarning, "cluster", "Error while reading JSON-RPC message for endpoint '" + GetName() + "': " + boost::diagnostic_information(ex));

			m_Client.reset();

			return;
		}

		OnMessageReceived(GetSelf(), message);
	}
}

/**
 * Gets the node address for this endpoint.
 *
 * @returns The node address (hostname).
 */
String Endpoint::GetHost(void) const
{
	return m_Host;
}

/**
 * Gets the service name for this endpoint.
 *
 * @returns The service name (port).
 */
String Endpoint::GetPort(void) const
{
	return m_Port;
}

Array::Ptr Endpoint::GetConfigFiles(void) const
{
	return m_ConfigFiles;
}

Array::Ptr Endpoint::GetAcceptConfig(void) const
{
	return m_AcceptConfig;
}

double Endpoint::GetSeen(void) const
{
	return m_Seen;
}

void Endpoint::SetSeen(double ts)
{
	m_Seen = ts;
}

double Endpoint::GetLocalLogPosition(void) const
{
	return m_LocalLogPosition;
}

void Endpoint::SetLocalLogPosition(double ts)
{
	m_LocalLogPosition = ts;
}

double Endpoint::GetRemoteLogPosition(void) const
{
	return m_RemoteLogPosition;
}

void Endpoint::SetRemoteLogPosition(double ts)
{
	m_RemoteLogPosition = ts;
}

bool Endpoint::IsSyncing(void) const
{
	return m_Syncing;
}

void Endpoint::SetSyncing(bool syncing)
{
	m_Syncing = syncing;
}

Dictionary::Ptr Endpoint::GetFeatures(void) const
{
	return m_Features;
}

void Endpoint::SetFeatures(const Dictionary::Ptr& features)
{
	m_Features = features;
}

bool Endpoint::HasFeature(const String& type) const
{
	Dictionary::Ptr features = GetFeatures();

	if (!features)
		return false;

	return features->Get(type);
}

void Endpoint::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("host", m_Host);
		bag->Set("port", m_Port);
		bag->Set("config_files", m_ConfigFiles);
		bag->Set("accept_config", m_AcceptConfig);
	}

	if (attributeTypes & Attribute_State) {
		bag->Set("seen", m_Seen);
		bag->Set("local_log_position", m_LocalLogPosition);
		bag->Set("remote_log_position", m_RemoteLogPosition);
		bag->Set("features", m_Features);
	}
}

void Endpoint::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_Host = bag->Get("host");
		m_Port = bag->Get("port");
		m_ConfigFiles = bag->Get("config_files");
		m_AcceptConfig = bag->Get("accept_config");
	}

	if (attributeTypes & Attribute_State) {
		m_Seen = bag->Get("seen");
		m_LocalLogPosition = bag->Get("local_log_position");
		m_RemoteLogPosition = bag->Get("remote_log_position");
		m_Features = bag->Get("features");
	}
}
