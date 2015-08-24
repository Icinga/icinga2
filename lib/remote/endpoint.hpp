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

#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "remote/i2-remote.hpp"
#include "remote/endpoint.thpp"
#include <set>

namespace icinga
{

class ApiClient;
class Zone;

/**
 * An endpoint that can be used to send and receive messages.
 *
 * @ingroup remote
 */
class I2_REMOTE_API Endpoint : public ObjectImpl<Endpoint>
{
public:
	DECLARE_OBJECT(Endpoint);
	DECLARE_OBJECTNAME(Endpoint);

	static boost::signals2::signal<void(const Endpoint::Ptr&, const intrusive_ptr<ApiClient>&)> OnConnected;
	static boost::signals2::signal<void(const Endpoint::Ptr&, const intrusive_ptr<ApiClient>&)> OnDisconnected;

	void AddClient(const intrusive_ptr<ApiClient>& client);
	void RemoveClient(const intrusive_ptr<ApiClient>& client);
	std::set<intrusive_ptr<ApiClient> > GetClients(void) const;

	intrusive_ptr<Zone> GetZone(void) const;

	bool IsConnected(void) const;

	static Endpoint::Ptr GetLocalEndpoint(void);

protected:
	virtual void OnAllConfigLoaded(void);
	virtual void ValidatePort(const String& value, const ValidationUtils& utils) override;

private:
	mutable boost::mutex m_ClientsLock;
	std::set<intrusive_ptr<ApiClient> > m_Clients;
	intrusive_ptr<Zone> m_Zone;
};

}

#endif /* ENDPOINT_H */
