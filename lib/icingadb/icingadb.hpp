/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ICINGADB_H
#define ICINGADB_H

#include "icingadb/icingadb-ti.hpp"
#include "icingadb/redisconnection.hpp"
#include "base/atomic.hpp"
#include "base/bulker.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/checkable.hpp"
#include "icinga/service.hpp"
#include "icinga/downtime.hpp"
#include "remote/messageorigin.hpp"
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <utility>

namespace icinga
{

/**
 * @ingroup icingadb
 */
class IcingaDB : public ObjectImpl<IcingaDB>
{
public:
	DECLARE_OBJECT(IcingaDB);
	DECLARE_OBJECTNAME(IcingaDB);

	IcingaDB();

	static void ConfigStaticInitialize();

	void Validate(int types, const ValidationUtils& utils) override;
	virtual void Start(bool runtimeCreated) override;
	virtual void Stop(bool runtimeRemoved) override;

	String GetEnvironmentId() const override;

	inline RedisConnection::Ptr GetConnection()
	{
		return m_RconLocked.load();
	}

	template<class T>
	static void AddKvsToMap(const Array::Ptr& kvs, T& map)
	{
		Value* key = nullptr;
		ObjectLock oLock (kvs);

		for (auto& kv : kvs) {
			if (key) {
				map.emplace(std::move(*key), std::move(kv));
				key = nullptr;
			} else {
				key = &kv;
			}
		}
	}

protected:
	void ValidateTlsProtocolmin(const Lazy<String>& lvalue, const ValidationUtils& utils) override;
	void ValidateConnectTimeout(const Lazy<double>& lvalue, const ValidationUtils& utils) override;

private:
	class DumpedGlobals
	{
	public:
		void Reset();
		bool IsNew(const String& id);

	private:
		std::set<String> m_Ids;
		std::mutex m_Mutex;
	};

	enum StateUpdate
	{
		Volatile    = 1ull << 0,
		RuntimeOnly = 1ull << 1,
		Full        = Volatile | RuntimeOnly,
	};

	void OnConnectedHandler();

	void PublishStatsTimerHandler();
	void PublishStats();

	/* config & status dump */
	void UpdateAllConfigObjects();
	std::vector<std::vector<intrusive_ptr<ConfigObject>>> ChunkObjects(std::vector<intrusive_ptr<ConfigObject>> objects, size_t chunkSize);
	void DeleteKeys(const RedisConnection::Ptr& conn, const std::vector<String>& keys, RedisConnection::QueryPriority priority);
	std::vector<String> GetTypeOverwriteKeys(const String& type);
	std::vector<String> GetTypeDumpSignalKeys(const Type::Ptr& type);
	void InsertObjectDependencies(const ConfigObject::Ptr& object, const String typeName, std::map<String, std::vector<String>>& hMSets,
			std::vector<Dictionary::Ptr>& runtimeUpdates, bool runtimeUpdate);
	void UpdateState(const Checkable::Ptr& checkable, StateUpdate mode);
	void SendConfigUpdate(const ConfigObject::Ptr& object, bool runtimeUpdate);
	void CreateConfigUpdate(const ConfigObject::Ptr& object, const String type, std::map<String, std::vector<String>>& hMSets,
			std::vector<Dictionary::Ptr>& runtimeUpdates, bool runtimeUpdate);
	void SendConfigDelete(const ConfigObject::Ptr& object);
	void SendStateChange(const ConfigObject::Ptr& object, const CheckResult::Ptr& cr, StateType type);
	void AddObjectDataToRuntimeUpdates(std::vector<Dictionary::Ptr>& runtimeUpdates, const String& objectKey,
			const String& redisKey, const Dictionary::Ptr& data);
	void DeleteRelationship(const String& id, const String& redisKeyWithoutPrefix, bool hasChecksum = false);

	void SendSentNotification(
		const Notification::Ptr& notification, const Checkable::Ptr& checkable, const std::set<User::Ptr>& users,
		NotificationType type, const CheckResult::Ptr& cr, const String& author, const String& text, double sendTime
	);

	void SendStartedDowntime(const Downtime::Ptr& downtime);
	void SendRemovedDowntime(const Downtime::Ptr& downtime);
	void SendAddedComment(const Comment::Ptr& comment);
	void SendRemovedComment(const Comment::Ptr& comment);
	void SendFlappingChange(const Checkable::Ptr& checkable, double changeTime, double flappingLastChange);
	void SendNextUpdate(const Checkable::Ptr& checkable);
	void SendAcknowledgementSet(const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type, bool persistent, double changeTime, double expiry);
	void SendAcknowledgementCleared(const Checkable::Ptr& checkable, const String& removedBy, double changeTime, double ackLastChange);
	void SendNotificationUsersChanged(const Notification::Ptr& notification, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	void SendNotificationUserGroupsChanged(const Notification::Ptr& notification, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	void SendTimePeriodRangesChanged(const TimePeriod::Ptr& timeperiod, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues);
	void SendTimePeriodIncludesChanged(const TimePeriod::Ptr& timeperiod, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	void SendTimePeriodExcludesChanged(const TimePeriod::Ptr& timeperiod, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	template<class T>
	void SendGroupsChanged(const ConfigObject::Ptr& command, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	void SendCommandEnvChanged(const ConfigObject::Ptr& command, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues);
	void SendCommandArgumentsChanged(const ConfigObject::Ptr& command, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues);
	void SendCustomVarsChanged(const ConfigObject::Ptr& object, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues);

	void ForwardHistoryEntries();

	std::vector<String> UpdateObjectAttrs(const ConfigObject::Ptr& object, int fieldType, const String& typeNameOverride);
	Dictionary::Ptr SerializeState(const Checkable::Ptr& checkable);

	/* Stats */
	Dictionary::Ptr GetStats();

	/* utilities */
	static String FormatCheckSumBinary(const String& str);
	static String FormatCommandLine(const Value& commandLine);
	static long long TimestampToMilliseconds(double timestamp);
	static String IcingaToStreamValue(const Value& value);
	static std::vector<Value> GetArrayDeletedValues(const Array::Ptr& arrayOld, const Array::Ptr& arrayNew);
	static std::vector<String> GetDictionaryDeletedKeys(const Dictionary::Ptr& dictOld, const Dictionary::Ptr& dictNew);

	static String GetObjectIdentifier(const ConfigObject::Ptr& object);
	static String CalcEventID(const char* eventType, const ConfigObject::Ptr& object, double eventTime = 0, NotificationType nt = NotificationType(0));
	static const char* GetNotificationTypeByEnum(NotificationType type);
	static Dictionary::Ptr SerializeVars(const Dictionary::Ptr& vars);

	static String HashValue(const Value& value);
	static String HashValue(const Value& value, const std::set<String>& propertiesBlacklist, bool propertiesWhitelist = false);

	static String GetLowerCaseTypeNameDB(const ConfigObject::Ptr& obj);
	static bool PrepareObject(const ConfigObject::Ptr& object, Dictionary::Ptr& attributes, Dictionary::Ptr& checkSums);

	static void ReachabilityChangeHandler(const std::set<Checkable::Ptr>& children);
	static void StateChangeHandler(const ConfigObject::Ptr& object, const CheckResult::Ptr& cr, StateType type);
	static void VersionChangedHandler(const ConfigObject::Ptr& object);
	static void DowntimeStartedHandler(const Downtime::Ptr& downtime);
	static void DowntimeRemovedHandler(const Downtime::Ptr& downtime);

	static void NotificationSentToAllUsersHandler(
		const Notification::Ptr& notification, const Checkable::Ptr& checkable, const std::set<User::Ptr>& users,
		NotificationType type, const CheckResult::Ptr& cr, const String& author, const String& text
	);

	static void CommentAddedHandler(const Comment::Ptr& comment);
	static void CommentRemovedHandler(const Comment::Ptr& comment);
	static void FlappingChangeHandler(const Checkable::Ptr& checkable, double changeTime);
	static void NewCheckResultHandler(const Checkable::Ptr& checkable);
	static void NextCheckChangedHandler(const Checkable::Ptr& checkable);
	static void HostProblemChangedHandler(const Service::Ptr& service);
	static void AcknowledgementSetHandler(const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type, bool persistent, double changeTime, double expiry);
	static void AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const String& removedBy, double changeTime);
	static void NotificationUsersChangedHandler(const Notification::Ptr& notification, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	static void NotificationUserGroupsChangedHandler(const Notification::Ptr& notification, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	static void TimePeriodRangesChangedHandler(const TimePeriod::Ptr& timeperiod, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues);
	static void TimePeriodIncludesChangedHandler(const TimePeriod::Ptr& timeperiod, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	static void TimePeriodExcludesChangedHandler(const TimePeriod::Ptr& timeperiod, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	static void UserGroupsChangedHandler(const User::Ptr& user, const Array::Ptr&, const Array::Ptr& newValues);
	static void HostGroupsChangedHandler(const Host::Ptr& host, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	static void ServiceGroupsChangedHandler(const Service::Ptr& service, const Array::Ptr& oldValues, const Array::Ptr& newValues);
	static void CommandEnvChangedHandler(const ConfigObject::Ptr& command, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues);
	static void CommandArgumentsChangedHandler(const ConfigObject::Ptr& command, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues);
	static void CustomVarsChangedHandler(const ConfigObject::Ptr& object, const Dictionary::Ptr& oldValues, const Dictionary::Ptr& newValues);

	void AssertOnWorkQueue();

	void ExceptionHandler(boost::exception_ptr exp);

	static std::vector<Type::Ptr> GetTypes();

	static void InitEnvironmentId();
	static void PersistEnvironmentId();

	Timer::Ptr m_StatsTimer;
	WorkQueue m_WorkQueue{0, 1, LogNotice};

	std::future<void> m_HistoryThread;
	Bulker<RedisConnection::Query> m_HistoryBulker {4096, std::chrono::milliseconds(250)};

	String m_PrefixConfigObject;
	String m_PrefixConfigCheckSum;

	bool m_ConfigDumpInProgress;
	bool m_ConfigDumpDone;

	RedisConnection::Ptr m_Rcon;
	// m_RconLocked containes a copy of the value in m_Rcon where all accesses are guarded by a mutex to allow safe
	// concurrent access like from the icingadb check command. It's a copy to still allow fast access without additional
	// syncronization to m_Rcon within the IcingaDB feature itself.
	Locked<RedisConnection::Ptr> m_RconLocked;
	std::unordered_map<ConfigType*, RedisConnection::Ptr> m_Rcons;
	std::atomic_size_t m_PendingRcons;

	struct {
		DumpedGlobals CustomVar, ActionUrl, NotesUrl, IconImage;
	} m_DumpedGlobals;

	// m_EnvironmentId is shared across all IcingaDB objects (typically there is at most one, but it is perfectly fine
	// to have multiple ones). It is initialized once (synchronized using m_EnvironmentIdInitMutex). After successful
	// initialization, the value is read-only and can be accessed without further synchronization.
	static String m_EnvironmentId;
	static std::mutex m_EnvironmentIdInitMutex;
};
}

#endif /* ICINGADB_H */
