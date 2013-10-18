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

#ifndef ENDPOINT_H
#define ENDPOINT_H

#include "base/dynamicobject.h"
#include "base/stream.h"
#include "base/array.h"
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

	Endpoint(void);

	static boost::signals2::signal<void (const Endpoint::Ptr&)> OnConnected;
	static boost::signals2::signal<void (const Endpoint::Ptr&, const Dictionary::Ptr&)> OnMessageReceived;

	Stream::Ptr GetClient(void) const;
	void SetClient(const Stream::Ptr& client);

	bool IsConnected(void) const;

	void SendMessage(const Dictionary::Ptr& request);

	String GetHost(void) const;
	String GetPort(void) const;
	Array::Ptr GetConfigFiles(void) const;
	Array::Ptr GetAcceptConfig(void) const;

	double GetSeen(void) const;
	void SetSeen(double ts);

	double GetLocalLogPosition(void) const;
	void SetLocalLogPosition(double ts);

	double GetRemoteLogPosition(void) const;
	void SetRemoteLogPosition(double ts);

	Dictionary::Ptr GetFeatures(void) const;
	void SetFeatures(const Dictionary::Ptr& features);

	bool HasFeature(const String& type) const;

	bool IsSyncing(void) const;
	void SetSyncing(bool syncing);

protected:
	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

private:
	Dictionary::Ptr m_Subscriptions;
	String m_Host;
	String m_Port;
	Array::Ptr m_ConfigFiles;
	Array::Ptr m_AcceptConfig;

	Stream::Ptr m_Client;
	double m_Seen;
	double m_LocalLogPosition;
	double m_RemoteLogPosition;
	Dictionary::Ptr m_Features;
	bool m_Syncing;

	boost::thread m_Thread;

	void MessageThreadProc(const Stream::Ptr& stream);
};

}

#endif /* ENDPOINT_H */
