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

#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "remote/endpoint.th"
#include "base/stream.h"
#include "base/array.h"
#include "remote/i2-remote.h"
#include <boost/signals2.hpp>

namespace icinga
{

class EndpointManager;

/**
 * An endpoint that can be used to send and receive messages.
 *
 * @ingroup cluster
 */
class I2_REMOTE_API Endpoint : public ObjectImpl<Endpoint>
{
public:
	DECLARE_PTR_TYPEDEFS(Endpoint);
	DECLARE_TYPENAME(Endpoint);

	static boost::signals2::signal<void (const Endpoint::Ptr&)> OnConnected;
        static boost::signals2::signal<void (const Endpoint::Ptr&)> OnDisconnected;
	static boost::signals2::signal<void (const Endpoint::Ptr&, const Dictionary::Ptr&)> OnMessageReceived;

	Stream::Ptr GetClient(void) const;
	void SetClient(const Stream::Ptr& client);

	bool IsConnected(void) const;
	bool IsAvailable(void) const;

	void SendMessage(const Dictionary::Ptr& request);

	bool HasFeature(const String& type) const;

private:
	Stream::Ptr m_Client;
	boost::thread m_Thread;
	Array::Ptr m_ConnectedEndpoints;

	void MessageThreadProc(const Stream::Ptr& stream);
};

}

#endif /* ENDPOINT_H */
