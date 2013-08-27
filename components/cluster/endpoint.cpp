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
	ObjectLock olock(this);

	return m_Client;
}

void Endpoint::SetClient(const Stream::Ptr& client)
{
	{
		ObjectLock olock(this);

		m_Client = client;
	}

	boost::thread thread(boost::bind(&Endpoint::MessageThreadProc, this, client));
	thread.detach();

	OnConnected(GetSelf());
}

void Endpoint::SendMessage(const Dictionary::Ptr& message)
{
	if (!IsConnected()) {
		// TODO: persist the message
		return;
	}

	try {
		JsonRpc::SendMessage(GetClient(), message);
	} catch (const std::exception& ex) {
		std::ostringstream msgbuf;
		msgbuf << "Error while sending JSON-RPC message for endpoint '" << GetName() << "': " << boost::diagnostic_information(ex);
		Log(LogWarning, "cluster", msgbuf.str());

		m_Client.reset();
	}
}

void Endpoint::MessageThreadProc(const Stream::Ptr& stream)
{
	for (;;) {
		Dictionary::Ptr message;

		try {
			message = JsonRpc::ReadMessage(stream);
		} catch (const std::exception& ex) {
			Log(LogWarning, "cluster", "Error while reading JSON-RPC message for endpoint '" + GetName() + "': " + boost::diagnostic_information(ex));

			m_Client.reset();

			return;
		}

		Utility::QueueAsyncCallback(bind(boost::ref(Endpoint::OnMessageReceived), GetSelf(), message));
	}
}

/**
 * Gets the node address for this endpoint.
 *
 * @returns The node address (hostname).
 */
String Endpoint::GetHost(void) const
{
	ObjectLock olock(this);

	return m_Host;
}

/**
 * Gets the service name for this endpoint.
 *
 * @returns The service name (port).
 */
String Endpoint::GetPort(void) const
{
	ObjectLock olock(this);

	return m_Port;
}

void Endpoint::InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const
{
	DynamicObject::InternalSerialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		bag->Set("local", m_Local);
		bag->Set("host", m_Host);
		bag->Set("port", m_Port);
	}
}

void Endpoint::InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes)
{
	DynamicObject::InternalDeserialize(bag, attributeTypes);

	if (attributeTypes & Attribute_Config) {
		m_Local = bag->Get("local");
		m_Host = bag->Get("host");
		m_Port = bag->Get("port");
	}
}
