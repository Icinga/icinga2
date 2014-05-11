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

#ifndef APILISTENER_H
#define APILISTENER_H

#include "remote/apilistener.th"
#include "remote/apiclient.h"
#include "remote/endpoint.h"
#include "remote/messageorigin.h"
#include "base/dynamicobject.h"
#include "base/timer.h"
#include "base/workqueue.h"
#include "base/tcpsocket.h"
#include "base/tlsstream.h"
#include <set>

namespace icinga
{

class ApiClient;

/**
* @ingroup remote
*/
class I2_REMOTE_API ApiListener : public ObjectImpl<ApiListener>
{
public:
	DECLARE_PTR_TYPEDEFS(ApiListener);
	DECLARE_TYPENAME(ApiListener);

	static boost::signals2::signal<void(bool)> OnMasterChanged;

	static ApiListener::Ptr GetInstance(void);

	shared_ptr<SSL_CTX> GetSSLContext(void) const;

	Endpoint::Ptr GetMaster(void) const;
	bool IsMaster(void) const;

	static String GetApiDir(void);

	void RelayMessage(const MessageOrigin& origin, const DynamicObject::Ptr& secobj, const Dictionary::Ptr& message, bool log);

	static Value StatsFunc(Dictionary::Ptr& status, Dictionary::Ptr& perfdata);
	std::pair<Dictionary::Ptr, Dictionary::Ptr> GetStatus(void);

	void AddAnonymousClient(const ApiClient::Ptr& aclient);
	void RemoveAnonymousClient(const ApiClient::Ptr& aclient);
	std::set<ApiClient::Ptr> GetAnonymousClients(void) const;

protected:
	virtual void OnConfigLoaded(void);
	virtual void Start(void);

private:
	shared_ptr<SSL_CTX> m_SSLContext;
	std::set<TcpSocket::Ptr> m_Servers;
	std::set<ApiClient::Ptr> m_AnonymousClients;
	Timer::Ptr m_Timer;

	void ApiTimerHandler(void);

	void AddListener(const String& service);
	void AddConnection(const String& node, const String& service);

	void NewClientHandler(const Socket::Ptr& client, ConnectionRole role);
	void ListenerThreadProc(const Socket::Ptr& server);

	void MessageHandler(const TlsStream::Ptr& sender, const String& identity, const Dictionary::Ptr& message);

	WorkQueue m_RelayQueue;
	WorkQueue m_LogQueue;

	boost::mutex m_LogLock;
	Stream::Ptr m_LogFile;
	size_t m_LogMessageCount;

	void SyncRelayMessage(const MessageOrigin& origin, const DynamicObject::Ptr& secobj, const Dictionary::Ptr& message, bool log);
	void PersistMessage(const Dictionary::Ptr& message);

	void OpenLogFile(void);
	void RotateLogFile(void);
	void CloseLogFile(void);
	static void LogGlobHandler(std::vector<int>& files, const String& file);
	void ReplayLog(const ApiClient::Ptr& client);
};

}

#endif /* APILISTENER_H */
