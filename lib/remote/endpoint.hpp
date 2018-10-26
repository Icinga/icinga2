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

#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "remote/i2-remote.hpp"
#include "remote/endpoint.thpp"
#include "base/ringbuffer.hpp"
#include <set>

namespace icinga
{

class JsonRpcConnection;
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

	Endpoint(void);

	static boost::signals2::signal<void(const Endpoint::Ptr&, const intrusive_ptr<JsonRpcConnection>&)> OnConnected;
	static boost::signals2::signal<void(const Endpoint::Ptr&, const intrusive_ptr<JsonRpcConnection>&)> OnDisconnected;

	void AddClient(const intrusive_ptr<JsonRpcConnection>& client);
	void RemoveClient(const intrusive_ptr<JsonRpcConnection>& client);
	std::set<intrusive_ptr<JsonRpcConnection> > GetClients(void) const;

	intrusive_ptr<Zone> GetZone(void) const;

	virtual bool GetConnected(void) const override;

	static Endpoint::Ptr GetLocalEndpoint(void);

	void SetCachedZone(const intrusive_ptr<Zone>& zone);

	void AddMessageSent(int bytes);
	void AddMessageReceived(int bytes);

	double GetMessagesSentPerSecond(void) const override;
	double GetMessagesReceivedPerSecond(void) const override;

	double GetBytesSentPerSecond(void) const override;
	double GetBytesReceivedPerSecond(void) const override;

protected:
	virtual void OnAllConfigLoaded(void) override;

private:
	mutable boost::mutex m_ClientsLock;
	std::set<intrusive_ptr<JsonRpcConnection> > m_Clients;
	intrusive_ptr<Zone> m_Zone;

	mutable RingBuffer m_MessagesSent;
	mutable RingBuffer m_MessagesReceived;
	mutable RingBuffer m_BytesSent;
	mutable RingBuffer m_BytesReceived;
};

}

#endif /* ENDPOINT_H */
