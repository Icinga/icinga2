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

#ifndef APILISTENER_H
#define APILISTENER_H

#include "remote/apilistener.thpp"
#include "remote/jsonrpcconnection.hpp"
#include "remote/httpconnection.hpp"
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"
#include "base/dynamicobject.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include "base/tcpsocket.hpp"
#include "base/tlsstream.hpp"
#include <set>

namespace icinga
{

class JsonRpcConnection;

/**
* @ingroup remote
*/
class I2_REMOTE_API ApiListener : public ObjectImpl<ApiListener>
{
public:
	DECLARE_OBJECT(ApiListener);
	DECLARE_OBJECTNAME(ApiListener);

	static boost::signals2::signal<void(bool)> OnMasterChanged;

	ApiListener(void);

	static ApiListener::Ptr GetInstance(void);

	boost::shared_ptr<SSL_CTX> GetSSLContext(void) const;

	Endpoint::Ptr GetMaster(void) const;
	bool IsMaster(void) const;

	static String GetApiDir(void);

	void SyncSendMessage(const Endpoint::Ptr& endpoint, const Dictionary::Ptr& message);
	void RelayMessage(const MessageOrigin& origin, const DynamicObject::Ptr& secobj, const Dictionary::Ptr& message, bool log);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);
	std::pair<Dictionary::Ptr, Dictionary::Ptr> GetStatus(void);

	void AddAnonymousClient(const JsonRpcConnection::Ptr& aclient);
	void RemoveAnonymousClient(const JsonRpcConnection::Ptr& aclient);
	std::set<JsonRpcConnection::Ptr> GetAnonymousClients(void) const;

	void AddHttpClient(const HttpConnection::Ptr& aclient);
	void RemoveHttpClient(const HttpConnection::Ptr& aclient);
	std::set<HttpConnection::Ptr> GetHttpClients(void) const;

	static Value ConfigUpdateHandler(const MessageOrigin& origin, const Dictionary::Ptr& params);

protected:
	virtual void OnConfigLoaded(void);
	virtual void OnAllConfigLoaded(void);
	virtual void Start(void);

private:
	boost::shared_ptr<SSL_CTX> m_SSLContext;
	std::set<TcpSocket::Ptr> m_Servers;
	std::set<JsonRpcConnection::Ptr> m_AnonymousClients;
	std::set<HttpConnection::Ptr> m_HttpClients;
	Timer::Ptr m_Timer;

	void ApiTimerHandler(void);

	bool AddListener(const String& node, const String& service);
	void AddConnection(const Endpoint::Ptr& endpoint);

	void NewClientHandler(const Socket::Ptr& client, const String& hostname, ConnectionRole role);
	void ListenerThreadProc(const Socket::Ptr& server);

	WorkQueue m_RelayQueue;

	boost::mutex m_LogLock;
	Stream::Ptr m_LogFile;
	size_t m_LogMessageCount;

	void SyncRelayMessage(const MessageOrigin& origin, const DynamicObject::Ptr& secobj, const Dictionary::Ptr& message, bool log);
	void PersistMessage(const Dictionary::Ptr& message, const DynamicObject::Ptr& secobj);

	void OpenLogFile(void);
	void RotateLogFile(void);
	void CloseLogFile(void);
	static void LogGlobHandler(std::vector<int>& files, const String& file);
	void ReplayLog(const JsonRpcConnection::Ptr& client);

	static Dictionary::Ptr LoadConfigDir(const String& dir);
	static bool UpdateConfigDir(const Dictionary::Ptr& oldConfig, const Dictionary::Ptr& newConfig, const String& configDir, bool authoritative);

	void SyncZoneDirs(void) const;
	void SyncZoneDir(const Zone::Ptr& zone) const;

	static bool IsConfigMaster(const Zone::Ptr& zone);
	static void ConfigGlobHandler(Dictionary::Ptr& config, const String& path, const String& file);
	void SendConfigUpdate(const JsonRpcConnection::Ptr& aclient);
};

}

#endif /* APILISTENER_H */
