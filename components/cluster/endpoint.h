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

#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "base/dynamicobject.h"
#include "base/stream.h"
#include "base/stdiostream.h"
#include <boost/signals2.hpp>

namespace icinga
{

class EndpointManager;

/**
 * An endpoint that can be used to send and receive messages.
 *
 * @ingroup cluster
 */
class Endpoint : public DynamicObject
{
public:
	DECLARE_PTR_TYPEDEFS(Endpoint);
	DECLARE_TYPENAME(Endpoint);

	static boost::signals2::signal<void (const Endpoint::Ptr&)> OnConnected;
	static boost::signals2::signal<void (const Endpoint::Ptr&, const Dictionary::Ptr&)> OnMessageReceived;

	Stream::Ptr GetClient(void) const;
	void SetClient(const Stream::Ptr& client);

	bool IsConnected(void) const;

	void SendMessage(const Dictionary::Ptr& request);

	String GetHost(void) const;
	String GetPort(void) const;

	double GetSeen(void) const;
	void SetSeen(double ts);

	String GetSpoolPath(void) const;

protected:
	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

	virtual void OnConfigLoaded(void);
	virtual void Stop(void);

private:
	Dictionary::Ptr m_Subscriptions;
	String m_Host;
	String m_Port;

	Stream::Ptr m_Client;
	double m_Seen;
	StdioStream::Ptr m_SpoolFile;

	void MessageThreadProc(const Stream::Ptr& stream);

	void OpenSpoolFile(void);
	void CloseSpoolFile(void);
};

}

#endif /* ENDPOINT_H */
