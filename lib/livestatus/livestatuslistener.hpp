// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef LIVESTATUSLISTENER_H
#define LIVESTATUSLISTENER_H

#include "livestatus/i2-livestatus.hpp"
#include "livestatus/livestatuslistener-ti.hpp"
#include "livestatus/livestatusquery.hpp"
#include "base/socket.hpp"
#include "base/wait-group.hpp"
#include <thread>

using namespace icinga;

namespace icinga
{

/**
 * @ingroup livestatus
 */
class LivestatusListener final : public ObjectImpl<LivestatusListener>
{
public:
	DECLARE_OBJECT(LivestatusListener);
	DECLARE_OBJECTNAME(LivestatusListener);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static int GetClientsConnected();
	static int GetConnections();

	void ValidateSocketType(const Lazy<String>& lvalue, const ValidationUtils& utils) override;

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	void ServerThreadProc();
	void ClientHandler(const Socket::Ptr& client);

	Socket::Ptr m_Listener;
	std::thread m_Thread;
	StoppableWaitGroup::Ptr m_WaitGroup = new StoppableWaitGroup();
};

}

#endif /* LIVESTATUSLISTENER_H */
