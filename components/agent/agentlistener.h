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

#ifndef AGENTLISTENER_H
#define AGENTLISTENER_H

#include "agent/agentlistener.th"
#include "base/dynamicobject.h"
#include "base/timer.h"
#include "base/array.h"
#include "base/tcpsocket.h"
#include "base/tlsstream.h"
#include "base/utility.h"
#include "base/tlsutility.h"
#include "icinga/service.h"

namespace icinga
{

/**
 * @ingroup agent
 */
class AgentListener : public ObjectImpl<AgentListener>
{
public:
	DECLARE_PTR_TYPEDEFS(AgentListener);
	DECLARE_TYPENAME(AgentListener);

	virtual void Start(void);

	shared_ptr<SSL_CTX> GetSSLContext(void) const;

private:
	shared_ptr<SSL_CTX> m_SSLContext;
	std::set<TcpSocket::Ptr> m_Servers;
	Timer::Ptr m_Timer;

	Timer::Ptr m_AgentTimer;
	void AgentTimerHandler(void);

	void AddListener(const String& service);
	void AddConnection(const String& node, const String& service);

	void NewClientHandler(const Socket::Ptr& client, TlsRole role);
	void ListenerThreadProc(const Socket::Ptr& server);

	void MessageHandler(const TlsStream::Ptr& sender, const String& identity, const Dictionary::Ptr& message);

	static String GetInventoryDir(void);

	friend class AgentCheckTask;
};

}

#endif /* AGENTLISTENER_H */
