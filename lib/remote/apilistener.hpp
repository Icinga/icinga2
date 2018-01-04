/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include "remote/httpserverconnection.hpp"
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"
#include "base/configobject.hpp"
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
struct ConfigDirInformation
{
	Dictionary::Ptr UpdateV1;
	Dictionary::Ptr UpdateV2;
};

/**
* @ingroup remote
*/
class ApiListener final : public ObjectImpl<ApiListener>
{
public:
	DECLARE_OBJECT(ApiListener);
	DECLARE_OBJECTNAME(ApiListener);

	static boost::signals2::signal<void(bool)> OnMasterChanged;

	ApiListener(void);

	static String GetApiDir(void);
	static String GetCertsDir(void);
	static String GetCaDir(void);
	static String GetCertificateRequestsDir(void);

	void UpdateSSLContext(void);

	static ApiListener::Ptr GetInstance(void);

	Endpoint::Ptr GetMaster(void) const;
	bool IsMaster(void) const;

	Endpoint::Ptr GetLocalEndpoint(void) const;

	void SyncSendMessage(const Endpoint::Ptr& endpoint, const Dictionary::Ptr& message);
	void RelayMessage(const MessageOrigin::Ptr& origin, const ConfigObject::Ptr& secobj, const Dictionary::Ptr& message, bool log);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);
	std::pair<Dictionary::Ptr, Dictionary::Ptr> GetStatus(void);

	void AddAnonymousClient(const JsonRpcConnection::Ptr& aclient);
	void RemoveAnonymousClient(const JsonRpcConnection::Ptr& aclient);
	std::set<JsonRpcConnection::Ptr> GetAnonymousClients(void) const;

	void AddHttpClient(const HttpServerConnection::Ptr& aclient);
	void RemoveHttpClient(const HttpServerConnection::Ptr& aclient);
	std::set<HttpServerConnection::Ptr> GetHttpClients(void) const;

	static double CalculateZoneLag(const Endpoint::Ptr& endpoint);

	/* filesync */
	static Value ConfigUpdateHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	/* configsync */
	static void ConfigUpdateObjectHandler(const ConfigObject::Ptr& object, const Value& cookie);
	static Value ConfigUpdateObjectAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static Value ConfigDeleteObjectAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static Value HelloAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void UpdateObjectAuthority(void);

	static bool IsHACluster(void);
	static String GetFromZoneName(const Zone::Ptr& fromZone);

	static String GetDefaultCertPath(void);
	static String GetDefaultKeyPath(void);
	static String GetDefaultCaPath(void);

protected:
	virtual void OnConfigLoaded(void) override;
	virtual void OnAllConfigLoaded(void) override;
	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeDeleted) override;

	virtual void ValidateTlsProtocolmin(const String& value, const ValidationUtils& utils) override;

private:
	std::shared_ptr<SSL_CTX> m_SSLContext;
	std::set<TcpSocket::Ptr> m_Servers;

	mutable boost::mutex m_AnonymousClientsLock;
	mutable boost::mutex m_HttpClientsLock;
	std::set<JsonRpcConnection::Ptr> m_AnonymousClients;
	std::set<HttpServerConnection::Ptr> m_HttpClients;

	Timer::Ptr m_Timer;
	Timer::Ptr m_ReconnectTimer;
	Timer::Ptr m_AuthorityTimer;
	Timer::Ptr m_CleanupCertificateRequestsTimer;
	Endpoint::Ptr m_LocalEndpoint;

	static ApiListener::Ptr m_Instance;

	void ApiTimerHandler(void);
	void ApiReconnectTimerHandler(void);
	void CleanupCertificateRequestsTimerHandler(void);

	bool AddListener(const String& node, const String& service);
	void AddConnection(const Endpoint::Ptr& endpoint);

	void NewClientHandler(const Socket::Ptr& client, const String& hostname, ConnectionRole role);
	void NewClientHandlerInternal(const Socket::Ptr& client, const String& hostname, ConnectionRole role);
	void ListenerThreadProc(const Socket::Ptr& server);

	WorkQueue m_RelayQueue;
	WorkQueue m_SyncQueue;

	boost::mutex m_LogLock;
	Stream::Ptr m_LogFile;
	size_t m_LogMessageCount;

	bool RelayMessageOne(const Zone::Ptr& zone, const MessageOrigin::Ptr& origin, const Dictionary::Ptr& message, const Endpoint::Ptr& currentMaster);
	void SyncRelayMessage(const MessageOrigin::Ptr& origin, const ConfigObject::Ptr& secobj, const Dictionary::Ptr& message, bool log);
	void PersistMessage(const Dictionary::Ptr& message, const ConfigObject::Ptr& secobj);

	void OpenLogFile(void);
	void RotateLogFile(void);
	void CloseLogFile(void);
	static void LogGlobHandler(std::vector<int>& files, const String& file);
	void ReplayLog(const JsonRpcConnection::Ptr& client);

	static void CopyCertificateFile(const String& oldCertPath, const String& newCertPath);

	/* filesync */
	static ConfigDirInformation LoadConfigDir(const String& dir);
	static Dictionary::Ptr MergeConfigUpdate(const ConfigDirInformation& config);
	static bool UpdateConfigDir(const ConfigDirInformation& oldConfig, const ConfigDirInformation& newConfig, const String& configDir, bool authoritative);

	void SyncZoneDirs(void) const;
	void SyncZoneDir(const Zone::Ptr& zone) const;

	static void ConfigGlobHandler(ConfigDirInformation& config, const String& path, const String& file);
	void SendConfigUpdate(const JsonRpcConnection::Ptr& aclient);

	/* configsync */
	void UpdateConfigObject(const ConfigObject::Ptr& object, const MessageOrigin::Ptr& origin,
		const JsonRpcConnection::Ptr& client = nullptr);
	void DeleteConfigObject(const ConfigObject::Ptr& object, const MessageOrigin::Ptr& origin,
		const JsonRpcConnection::Ptr& client = nullptr);
	void SendRuntimeConfigObjects(const JsonRpcConnection::Ptr& aclient);

	void SyncClient(const JsonRpcConnection::Ptr& aclient, const Endpoint::Ptr& endpoint, bool needSync);
};

}

#endif /* APILISTENER_H */
