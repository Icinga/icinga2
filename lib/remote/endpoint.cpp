/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "remote/endpoint.hpp"
#include "remote/endpoint.tcpp"
#include "remote/apilistener.hpp"
#include "remote/jsonrpcconnection.hpp"
#include "remote/zone.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_TYPE(Endpoint);

boost::signals2::signal<void(const Endpoint::Ptr&, const JsonRpcConnection::Ptr&)> Endpoint::OnConnected;
boost::signals2::signal<void(const Endpoint::Ptr&, const JsonRpcConnection::Ptr&)> Endpoint::OnDisconnected;

void Endpoint::OnAllConfigLoaded(void)
{
	ObjectImpl<Endpoint>::OnAllConfigLoaded();

	if (!m_Zone)
		BOOST_THROW_EXCEPTION(ScriptError("Endpoint '" + GetName() +
		    "' does not belong to a zone.", GetDebugInfo()));
}

void Endpoint::SetCachedZone(const Zone::Ptr& zone)
{
	if (m_Zone)
		BOOST_THROW_EXCEPTION(ScriptError("Endpoint '" + GetName()
		    + "' is in more than one zone.", GetDebugInfo()));

	m_Zone = zone;
}

void Endpoint::AddClient(const JsonRpcConnection::Ptr& client)
{
	bool was_master = ApiListener::GetInstance()->IsMaster();

	{
		boost::mutex::scoped_lock lock(m_ClientsLock);
		m_Clients.insert(client);
	}

	bool is_master = ApiListener::GetInstance()->IsMaster();

	if (was_master != is_master)
		ApiListener::OnMasterChanged(is_master);

	OnConnected(this, client);
}

void Endpoint::RemoveClient(const JsonRpcConnection::Ptr& client)
{
	bool was_master = ApiListener::GetInstance()->IsMaster();

	{
		boost::mutex::scoped_lock lock(m_ClientsLock);
		m_Clients.erase(client);

		Log(LogWarning, "ApiListener")
		    << "Removing API client for endpoint '" << GetName() << "'. " << m_Clients.size() << " API clients left.";

		SetConnecting(false);
	}

	bool is_master = ApiListener::GetInstance()->IsMaster();

	if (was_master != is_master)
		ApiListener::OnMasterChanged(is_master);

	OnDisconnected(this, client);
}

std::set<JsonRpcConnection::Ptr> Endpoint::GetClients(void) const
{
	boost::mutex::scoped_lock lock(m_ClientsLock);
	return m_Clients;
}

Zone::Ptr Endpoint::GetZone(void) const
{
	return m_Zone;
}

bool Endpoint::GetConnected(void) const
{
	boost::mutex::scoped_lock lock(m_ClientsLock);
	return !m_Clients.empty();
}

Endpoint::Ptr Endpoint::GetLocalEndpoint(void)
{
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (!listener)
		return nullptr;

	return listener->GetLocalEndpoint();
}
