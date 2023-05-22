/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#pragma once

#include "livestatus/i2-livestatus.hpp"
#include "livestatus/livestatuslistener-ti.hpp"
#include "livestatus/livestatusquery.hpp"
#include "base/socket.hpp"
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
};

}
