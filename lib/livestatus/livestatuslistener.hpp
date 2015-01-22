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

#ifndef LIVESTATUSLISTENER_H
#define LIVESTATUSLISTENER_H

#include "livestatus/livestatuslistener.thpp"
#include "livestatus/livestatusquery.hpp"
#include "base/socket.hpp"
#include <boost/thread/thread.hpp>

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class LivestatusListener : public ObjectImpl<LivestatusListener>
{
public:
	DECLARE_OBJECT(LivestatusListener);
	DECLARE_OBJECTNAME(LivestatusListener);

	static Value StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static int GetClientsConnected(void);
	static int GetConnections(void);

	static void ValidateSocketType(const String& location, const LivestatusListener::Ptr& object);

protected:
	virtual void Start(void);
	virtual void Stop(void);

private:
	void ServerThreadProc(void);
	void ClientHandler(const Socket::Ptr& client);

	Socket::Ptr m_Listener;
	boost::thread m_Thread;
};

}

#endif /* LIVESTATUSLISTENER_H */
