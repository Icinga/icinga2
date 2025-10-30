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
#include "icinga/command.hpp"
#include "icinga/service.hpp"
#include "icinga/downtime.hpp"
#include "remote/messageorigin.hpp"
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <atomic>
#include <chrono>
#include <future>
#include <memory>
#include <mutex>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace icinga
{

/**
 * RedisKey is the enumeration of all Redis keys used by IcingaDB.
 *
 * Each enum value represents a specific Redis key type from which the actual Redis key strings are derived.
 * For instance, the `Host` enum value corresponds to the Redis key pattern `icinga:host`. These enums help
 * in organizing and managing the various Redis keys in a transparent and consistent manner and avoid hardcoding
 * key strings throughout the codebase.
 *
 * @ingroup icingadb
 */
enum class RedisKey : uint8_t
{
	/* Command-related keys */
	CheckCmdArg,
	CheckCmdEnvVar,
	CheckCmdCustomVar,

	EventCmdArg,
	EventCmdEnvVar,
	EventCmdCustomVar,

	NotificationCmdArg,
	NotificationCmdEnvVar,
	NotificationCmdCustomVar,

	/* Dependency related config */
	DependencyNode,
	DependencyEdge,
	RedundancyGroup,

	/* Hosts & Services */
	HostCustomVar,
	HostGroupMember,
	HostGroupCustomVar,

	ServiceCustomVar,
	ServiceGroupMember,
	ServiceGroupCustomVar,

	/* Users & Usergroups */
	UserCustomVar,
	UserGroupMember,
	UserGroupCustomVar,

	/* Notification */
	NotificationUser,
	NotificationUserGroup,
	NotificationRecipient,
	NotificationCustomVar,

	/* Timeperiods */
	TimePeriodRange,
	TimePeriodInclude,
	TimePeriodExclude,
	TimePeriodCustomVar,

	/* Downtimes */
	ScheduledDowntimeCustomVar,

	/* State keys marker and state entries */
	_state_keys_begin,
	HostState,
	ServiceState,
	RedundancyGroupState,
	DependencyEdgeState,
	_state_keys_end,
};

/**
 * Dirty bits for config/state changes.
 *
 * These are used to mark objects as "dirty" in order to trigger appropriate updates in Redis.
 * Each bit represents a different type of change that requires a specific action to be taken.
 *
 * @ingroup icingadb
 */
enum DirtyBits : uint32_t
{
	ConfigUpdate  = 1<<0, // Trigger a Redis config update for the object.
	ConfigDelete  = 1<<1, // Send a deletion command for the object to Redis.
	VolatileState = 1<<2, // Send a volatile state update to Redis (affects only checkables).
	RuntimeState  = 1<<3, // Send a runtime state update to Redis (affects only checkables).
	NextUpdate    = 1<<4, // Update the `icinga:nextupdate:{host,service}` Redis keys (affects only checkables).

	FullState = VolatileState | RuntimeState, // A combination of all (non-dependency) state-related dirty bits.

	// All valid dirty bits combined used for masking input values.
	DirtyBitsAll = ConfigUpdate | ConfigDelete | FullState | NextUpdate
};

/**
 * A variant type representing the identifier of a pending item.
 *
 * This variant can hold either a string representing a real Redis hash key or a pair consisting of
 * a configuration object pointer and a dependency group pointer. A pending item identified by the
 * latter variant type operates primarily on the associated configuration object or dependency group,
 * thus the pairs are used for uniqueness in the pending items container.
 *
 * @ingroup icingadb
 */
using PendingItemKey = std::variant<std::string /* Redis hash keys */, std::pair<ConfigObject::Ptr, DependencyGroup::Ptr>>;

/**
 * A pending queue item.
 *
 * This struct represents a generic pending item in the queue that is associated with a unique identifier
 * and dirty bits indicating the type of updates required in Redis. The @c EnqueueTime field records the
 * time when the item was added to the queue, which can be useful for tracking how long an item waits before
 * being processed. This base struct is extended by more specific pending item types that operate on different
 * kinds of objects, such as configuration objects or dependency groups.
 *
 * @ingroup icingadb
 */
struct PendingQueueItem
{
	uint32_t DirtyBits;
	PendingItemKey ID;
	const std::chrono::steady_clock::time_point EnqueueTime;

	PendingQueueItem(PendingItemKey&& id, uint32_t dirtyBits);
};

/**
 * A pending configuration object item.
 *
 * This struct represents a pending item in the queue that is associated with a configuration object.
 * It contains a pointer to the configuration object and the dirty bits indicating the type of updates
 * required for that object in Redis. A pending configuration item operates primarily on config objects,
 * thus the @c ID field in the base struct is only used for uniqueness in the pending items container.
 *
 * @ingroup icingadb
 */
struct PendingConfigItem : PendingQueueItem
{
	ConfigObject::Ptr Object;

	PendingConfigItem(const ConfigObject::Ptr& obj, uint32_t bits);
};

/**
 * A pending dependency group state item.
 *
 * This struct represents a pending item in the queue that is associated with a dependency group.
 * It contains a pointer to the dependency group for which state updates are required. The dirty bits
 * in the base struct are not used for this item type, as the operation is specific to updating the
 * state of the dependency group itself.
 *
 * @ingroup icingadb
 */
struct PendingDependencyGroupStateItem : PendingQueueItem
{
	DependencyGroup::Ptr DepGroup;

	explicit PendingDependencyGroupStateItem(const DependencyGroup::Ptr& depGroup);
};

/**
 * A pending dependency edge item.
 *
 * This struct represents a pending dependency child registration into a dependency group.
 * It contains a pointer to the dependency group and the checkable child being registered.
 * The dirty bits in the base struct are not used for this item type, as the operation is specific
 * to registering the child into the dependency group.
 *
 * @ingroup icingadb
 */
struct PendingDependencyEdgeItem : PendingQueueItem
{
	DependencyGroup::Ptr DepGroup;
	Checkable::Ptr Child;

	PendingDependencyEdgeItem(const DependencyGroup::Ptr& depGroup, const Checkable::Ptr& child);
};

// Map of Redis keys to a boolean indicating whether to delete the checksum key as well.
using RelationsKeyMap = std::map<RedisKey, bool /* checksum? */>;

/**
 * A pending relations deletion item.
 *
 * This struct represents a pending item in the queue that is associated with the deletion of relations
 * in Redis. It contains a map of Redis keys from which the relation identified by the @c ID field should
 * be deleted. The @c ID field represents the unique identifier of the relation to be deleted, and the
 * @c Relations map specifies the Redis keys and whether to delete the corresponding checksum keys.
 *
 * @ingroup icingadb
 */
struct RelationsDeletionItem : PendingQueueItem
{
	RelationsKeyMap Relations;

	RelationsDeletionItem(const String& id, RelationsKeyMap relations);
};

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

	inline RedisConnection::Ptr GetConnection() const
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
	RedisConnInfo::ConstPtr GetRedisConnInfo() const;

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
	void InsertCheckableDependencies(const Checkable::Ptr& checkable, std::map<String, RedisConnection::Query>& hMSets,
		std::vector<Dictionary::Ptr>* runtimeUpdates, const DependencyGroup::Ptr& onlyDependencyGroup = nullptr);
	void InsertObjectDependencies(const ConfigObject::Ptr& object, const String typeName, std::map<String, std::vector<String>>& hMSets,
			std::vector<Dictionary::Ptr>& runtimeUpdates, bool runtimeUpdate);
	void UpdateDependenciesState(const Checkable::Ptr& checkable, const DependencyGroup::Ptr& onlyDependencyGroup = nullptr,
		std::set<DependencyGroup*>* seenGroups = nullptr) const;
	void UpdateState(const Checkable::Ptr& checkable, StateUpdate mode);
	void SendConfigUpdate(const ConfigObject::Ptr& object, bool runtimeUpdate);
	void CreateConfigUpdate(const ConfigObject::Ptr& object, const String type, std::map<String, std::vector<String>>& hMSets,
			std::vector<Dictionary::Ptr>& runtimeUpdates, bool runtimeUpdate);
	void SendConfigDelete(const ConfigObject::Ptr& object);
	void SendStateChange(const ConfigObject::Ptr& object, const CheckResult::Ptr& cr, StateType type);
	void AddObjectDataToRuntimeUpdates(std::vector<Dictionary::Ptr>& runtimeUpdates, const String& objectKey,
			const String& redisKey, const Dictionary::Ptr& data);
	void DeleteRelationship(const String& id, const String& redisKeyWithoutPrefix, bool hasChecksum = false);
	void DeleteRelationship(const String& id, RedisKey redisKey, bool hasChecksum = false);
	void DeleteState(const String& id, RedisKey redisKey, bool hasChecksum = false) const;
	void AddDataToHmSets(std::map<String, RedisConnection::Query>& hMSets, RedisKey redisKey, const String& id, const Dictionary::Ptr& data) const;

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
	void SendDependencyGroupChildRegistered(const Checkable::Ptr& child, const DependencyGroup::Ptr& dependencyGroup);
	void SendDependencyGroupChildRemoved(const DependencyGroup::Ptr& dependencyGroup, const std::vector<Dependency::Ptr>& dependencies, bool removeGroup);

	void ForwardHistoryEntries();

	std::vector<String> UpdateObjectAttrs(const ConfigObject::Ptr& object, int fieldType, const String& typeNameOverride);
	Dictionary::Ptr SerializeState(const Checkable::Ptr& checkable);

	/* Stats */
	static Dictionary::Ptr GetStats();

	/* utilities */
	static bool IsStateKey(RedisKey key);
	static String FormatCheckSumBinary(const String& str);
	static String FormatCommandLine(const Value& commandLine);
	static long long TimestampToMilliseconds(double timestamp);
	static String IcingaToStreamValue(const Value& value);
	static std::vector<Value> GetArrayDeletedValues(const Array::Ptr& arrayOld, const Array::Ptr& arrayNew);
	static std::vector<String> GetDictionaryDeletedKeys(const Dictionary::Ptr& dictOld, const Dictionary::Ptr& dictNew);

	static String GetObjectIdentifier(const ConfigObject::Ptr& object);
	static String CalcEventID(const char* eventType, const ConfigObject::Ptr& object, double eventTime = 0, NotificationType nt = NotificationType(0));
	static int StateFilterToRedisValue(int filter);
	static int TypeFilterToRedisValue(int filter);
	static const char* GetNotificationTypeByEnum(NotificationType type);
	static String CommentTypeToString(CommentType type);
	static Dictionary::Ptr SerializeVars(const Dictionary::Ptr& vars);
	static Dictionary::Ptr SerializeDependencyEdgeState(const DependencyGroup::Ptr& dependencyGroup, const Dependency::Ptr& dep);
	static Dictionary::Ptr SerializeRedundancyGroupState(const Checkable::Ptr& child, const DependencyGroup::Ptr& redundancyGroup);
	static String GetDependencyEdgeStateId(const DependencyGroup::Ptr& dependencyGroup, const Dependency::Ptr& dep);

	static String HashValue(const Value& value);
	static String HashValue(const Value& value, const std::set<String>& propertiesBlacklist, bool propertiesWhitelist = false);

	static String GetLowerCaseTypeNameDB(const ConfigObject::Ptr& obj);
	static bool PrepareObject(const ConfigObject::Ptr& object, Dictionary::Ptr& attributes);

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
	static void NextCheckUpdatedHandler(const Checkable::Ptr& checkable);
	static void DependencyGroupChildRegisteredHandler(const Checkable::Ptr& child, const DependencyGroup::Ptr& dependencyGroup);
	static void DependencyGroupChildRemovedHandler(const DependencyGroup::Ptr& dependencyGroup, const std::vector<Dependency::Ptr>& dependencies, bool removeGroup);
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

	static void ExecuteRedisTransaction(const RedisConnection::Ptr& rcon, std::map<String, RedisConnection::Query>& hMSets,
		const std::vector<Dictionary::Ptr>& runtimeUpdates);

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
	std::atomic_bool m_ConfigDumpDone;

	/**
	 * The primary Redis connection used to send history and heartbeat queries.
	 *
	 * This connection is used exclusively for sending history and heartbeat queries to Redis. It ensures that
	 * history and heartbeat operations do not interfere with other Redis operations. Also, it is the leader for
	 * all other Redis connections including @c m_RconWorker, and is the only source of truth for all IcingaDB Redis
	 * related connection statistics.
	 *
	 * Note: This will still be shared with the icingadb check command, as that command also sends
	 * only XREAD queries which are similar in nature to history/heartbeat queries.
	 */
	RedisConnection::Ptr m_Rcon;
	// m_RconLocked contains a copy of the value in m_Rcon where all accesses are guarded by a mutex to
	// allow safe concurrent access like from the icingadb check command. It's a copy to still allow fast access
	// without additional synchronization to m_Rcon within the IcingaDB feature itself.
	Locked<RedisConnection::Ptr> m_RconLocked;
	/**
	 * A Redis connection for general queries.
	 *
	 * This connection is used for all non-history and non-heartbeat related queries to Redis.
	 * It is a child of @c m_Rcon, meaning it forwards all its connection stats to @c m_Rcon as well.
	 */
	RedisConnection::Ptr m_RconWorker;
	std::unordered_map<ConfigType*, RedisConnection::Ptr> m_Rcons;
	std::atomic_size_t m_PendingRcons;

	struct {
		DumpedGlobals CustomVar, ActionUrl, NotesUrl, IconImage, DependencyGroup;
	} m_DumpedGlobals;

	// m_EnvironmentId is shared across all IcingaDB objects (typically there is at most one, but it is perfectly fine
	// to have multiple ones). It is initialized once (synchronized using m_EnvironmentIdInitMutex). After successful
	// initialization, the value is read-only and can be accessed without further synchronization.
	static String m_EnvironmentId;
	static std::mutex m_EnvironmentIdInitMutex;

	static std::unordered_set<Type*> m_IndexedTypes;

	// A variant type that can hold any of the pending item types used in the pending items container.
	using PendingItemVariant = std::variant<
		PendingConfigItem,
		PendingDependencyGroupStateItem,
		PendingDependencyEdgeItem,
		RelationsDeletionItem
	>;

	struct PendingItemKeyExtractor
	{
		// The type of the key extracted from a pending item required by Boost.MultiIndex.
		using result_type = const PendingItemKey&;

		result_type operator()(const PendingItemVariant& item) const
		{
			return std::visit([](const auto& pendingItem) -> result_type { return pendingItem.ID; }, item);
		}
	};

	// A multi-index container for managing pending items with unique IDs and maintaining insertion order.
	// The first index is an ordered unique index based on the pending item key, allowing for efficient
	// lookups and ensuring uniqueness of items. The second index is a sequenced index that maintains the
	// order of insertion, enabling FIFO processing of pending items.
	using PendingItemsSet = boost::multi_index_container<
		PendingItemVariant,
		boost::multi_index::indexed_by<
			boost::multi_index::ordered_unique<PendingItemKeyExtractor>, // std::variant has operator< defined.
			boost::multi_index::sequenced<>
		>
	>;

	std::thread m_PendingItemsThread; // The background worker thread (consumer of m_PendingItems).
	PendingItemsSet m_PendingItems; // Container for pending items with dirty bits (access protected by m_PendingItemsMutex).
	std::mutex m_PendingItemsMutex; // Mutex to protect access to m_PendingItems.
	std::condition_variable m_PendingItemsCV; // Condition variable to forcefully wake up the worker thread.

	void PendingItemsThreadProc();
	std::chrono::duration<double> DequeueAndProcessOne(std::unique_lock<std::mutex>& lock);
	void ProcessPendingItem(const PendingConfigItem& item);
	void ProcessPendingItem(const PendingDependencyGroupStateItem& item) const;
	void ProcessPendingItem(const PendingDependencyEdgeItem& item);
	void ProcessPendingItem(const RelationsDeletionItem& item);

	void EnqueueConfigObject(const ConfigObject::Ptr& object, uint32_t bits);
	void EnqueueDependencyGroupStateUpdate(const DependencyGroup::Ptr& depGroup);
	void EnqueueDependencyChildRegistered(const DependencyGroup::Ptr& depGroup, const Checkable::Ptr& child);
	void EnqueueDependencyChildRemoved(const DependencyGroup::Ptr& depGroup, const std::vector<Dependency::Ptr>& dependencies, bool removeGroup);
	void EnqueueRelationsDeletion(const String& id, const RelationsKeyMap& relations);
};
}

#endif /* ICINGADB_H */
