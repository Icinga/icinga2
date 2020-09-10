/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef APILISTENER_H
#define APILISTENER_H

#include "remote/apilistener-ti.hpp"
#include "remote/jsonrpcconnection.hpp"
#include "remote/httpserverconnection.hpp"
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"
#include "base/configobject.hpp"
#include "base/process.hpp"
#include "base/shared.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include "base/tcpsocket.hpp"
#include "base/tlsstream.hpp"
#include "base/threadpool.hpp"
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/asio/ssl/context.hpp>
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
	Dictionary::Ptr Checksums;
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

	ApiListener();

	static String GetApiDir();
	static String GetApiZonesDir();
	static String GetApiZonesStageDir();
	static String GetCertsDir();
	static String GetCaDir();
	static String GetCertificateRequestsDir();

	void UpdateSSLContext();

	static ApiListener::Ptr GetInstance();

	Endpoint::Ptr GetMaster() const;
	bool IsMaster() const;

	Endpoint::Ptr GetLocalEndpoint() const;

	void SyncSendMessage(const Endpoint::Ptr& endpoint, const Dictionary::Ptr& message);
	void RelayMessage(const MessageOrigin::Ptr& origin, const ConfigObject::Ptr& secobj, const Dictionary::Ptr& message, bool log);

	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);
	std::pair<Dictionary::Ptr, Dictionary::Ptr> GetStatus();

	bool AddAnonymousClient(const JsonRpcConnection::Ptr& aclient);
	void RemoveAnonymousClient(const JsonRpcConnection::Ptr& aclient);
	std::set<JsonRpcConnection::Ptr> GetAnonymousClients() const;

	void AddHttpClient(const HttpServerConnection::Ptr& aclient);
	void RemoveHttpClient(const HttpServerConnection::Ptr& aclient);
	std::set<HttpServerConnection::Ptr> GetHttpClients() const;

	static double CalculateZoneLag(const Endpoint::Ptr& endpoint);

	/* filesync */
	static Value ConfigUpdateHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static Value ConfigHaveZonesHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static Value ConfigWantZonesHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static Value ConfigHaveFilesHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static Value ConfigWantFilesHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static void HandleConfigUpdate(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	/* configsync */
	static void ConfigUpdateObjectHandler(const ConfigObject::Ptr& object, const Value& cookie);
	static Value ConfigUpdateObjectAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static Value ConfigDeleteObjectAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static Value ConfigHaveObjectsAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static Value ConfigWantObjectsAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	/* API config packages */
	void SetActivePackageStage(const String& package, const String& stage);
	String GetActivePackageStage(const String& package);
	void RemoveActivePackageStage(const String& package);

	static Value HelloAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void UpdateObjectAuthority();

	static bool IsHACluster();
	static String GetFromZoneName(const Zone::Ptr& fromZone);

	static String GetDefaultCertPath();
	static String GetDefaultKeyPath();
	static String GetDefaultCaPath();

	static inline
	bool UpdatedObjectAuthority()
	{
		return m_UpdatedObjectAuthority.load();
	}

	double GetTlsHandshakeTimeout() const override;
	void SetTlsHandshakeTimeout(double value, bool suppress_events, const Value& cookie) override;

protected:
	void OnConfigLoaded() override;
	void OnAllConfigLoaded() override;
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeDeleted) override;

	void ValidateTlsProtocolmin(const Lazy<String>& lvalue, const ValidationUtils& utils) override;
	void ValidateTlsHandshakeTimeout(const Lazy<double>& lvalue, const ValidationUtils& utils) override;

private:
	Shared<boost::asio::ssl::context>::Ptr m_SSLContext;

	mutable boost::mutex m_AnonymousClientsLock;
	mutable boost::mutex m_HttpClientsLock;
	std::set<JsonRpcConnection::Ptr> m_AnonymousClients;
	std::set<HttpServerConnection::Ptr> m_HttpClients;

	Timer::Ptr m_Timer;
	Timer::Ptr m_ReconnectTimer;
	Timer::Ptr m_AuthorityTimer;
	Timer::Ptr m_CleanupCertificateRequestsTimer;
	Timer::Ptr m_ApiPackageIntegrityTimer;

	Endpoint::Ptr m_LocalEndpoint;

	static ApiListener::Ptr m_Instance;
	static std::atomic<bool> m_UpdatedObjectAuthority;

	void ApiTimerHandler();
	void ApiReconnectTimerHandler();
	void CleanupCertificateRequestsTimerHandler();
	void CheckApiPackageIntegrity();

	bool AddListener(const String& node, const String& service);
	void AddConnection(const Endpoint::Ptr& endpoint);

	void NewClientHandler(
		boost::asio::yield_context yc, const Shared<boost::asio::io_context::strand>::Ptr& strand,
		const Shared<AsioTlsStream>::Ptr& client, const String& hostname, ConnectionRole role
	);
	void NewClientHandlerInternal(
		boost::asio::yield_context yc, const Shared<boost::asio::io_context::strand>::Ptr& strand,
		const Shared<AsioTlsStream>::Ptr& client, const String& hostname, ConnectionRole role
	);
	void ListenerCoroutineProc(boost::asio::yield_context yc, const Shared<boost::asio::ip::tcp::acceptor>::Ptr& server, const Shared<boost::asio::ssl::context>::Ptr& sslContext);

	WorkQueue m_RelayQueue;
	WorkQueue m_SyncQueue{0, 4};

	boost::mutex m_LogLock;
	Stream::Ptr m_LogFile;
	size_t m_LogMessageCount{0};

	bool RelayMessageOne(const Zone::Ptr& zone, const MessageOrigin::Ptr& origin, const Dictionary::Ptr& message, const Endpoint::Ptr& currentZoneMaster);
	void SyncRelayMessage(const MessageOrigin::Ptr& origin, const ConfigObject::Ptr& secobj, const Dictionary::Ptr& message, bool log);
	void PersistMessage(const Dictionary::Ptr& message, const ConfigObject::Ptr& secobj);

	void OpenLogFile();
	void RotateLogFile();
	void CloseLogFile();
	static void LogGlobHandler(std::vector<int>& files, const String& file);
	void ReplayLog(const JsonRpcConnection::Ptr& client);

	static void CopyCertificateFile(const String& oldCertPath, const String& newCertPath);

	void UpdateStatusFile(boost::asio::ip::tcp::endpoint localEndpoint);
	void RemoveStatusFile();

	/* filesync */
	static boost::mutex m_ConfigSyncStageLock;

	void SyncLocalZoneDirs() const;
	void SyncLocalZoneDir(const Zone::Ptr& zone) const;

	void SendConfigUpdate(const JsonRpcConnection::Ptr& aclient);
	void DeclareConfigUpdate(const JsonRpcConnection::Ptr& aclient);

	static Dictionary::Ptr MergeConfigUpdate(const ConfigDirInformation& config);

	static ConfigDirInformation LoadConfigDir(const String& dir, const Dictionary::Ptr& contents = nullptr);
	static void ConfigGlobHandler(ConfigDirInformation& config, const Dictionary::Ptr& contents, const String& path, const String& file);

	static void TryActivateZonesStageCallback(const ProcessResult& pr,
		const std::vector<String>& relativePaths);
	static void AsyncTryActivateZonesStage(const std::vector<String>& relativePaths, const Shared<boost::mutex::scoped_lock>::Ptr& lock);

	static String GetChecksum(const String& content);
	static bool CheckConfigChange(const ConfigDirInformation& oldConfig, const ConfigDirInformation& newConfig);

	void UpdateLastFailedZonesStageValidation(const String& log);
	void ClearLastFailedZonesStageValidation();

	/* configsync */
	void UpdateConfigObject(const ConfigObject::Ptr& object, const MessageOrigin::Ptr& origin,
		const JsonRpcConnection::Ptr& client = nullptr);
	void DeleteConfigObject(const ConfigObject::Ptr& object, const MessageOrigin::Ptr& origin,
		const JsonRpcConnection::Ptr& client = nullptr);
	void SendRuntimeConfigObjects(const JsonRpcConnection::Ptr& aclient);
	void DeclareRuntimeConfigObjects(const JsonRpcConnection::Ptr& client);

	void SyncClient(const JsonRpcConnection::Ptr& aclient, const Endpoint::Ptr& endpoint, bool needSync);

	/* API Config Packages */
	mutable boost::mutex m_ActivePackageStagesLock;
	std::map<String, String> m_ActivePackageStages;

	void UpdateActivePackageStagesCache();
};

}

#endif /* APILISTENER_H */
