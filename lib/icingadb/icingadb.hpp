/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef ICINGADB_H
#define ICINGADB_H

#include "icingadb/icingadb-ti.hpp"
#include "icingadb/redisconnection.hpp"
#include "base/timer.hpp"
#include "base/workqueue.hpp"
#include "icinga/customvarobject.hpp"
#include "icinga/checkable.hpp"
#include "icinga/service.hpp"
#include "icinga/downtime.hpp"
#include "remote/messageorigin.hpp"
#include <boost/thread/once.hpp>
#include <atomic>
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
	void UpdateState(const Checkable::Ptr& checkable);
	void SendConfigUpdate(const ConfigObject::Ptr& object, bool runtimeUpdate);
	void CreateConfigUpdate(const ConfigObject::Ptr& object, const String type, std::map<String, std::vector<String>>& hMSets,
			std::vector<Dictionary::Ptr>& runtimeUpdates, bool runtimeUpdate);
	void SendConfigDelete(const ConfigObject::Ptr& object);
	void SendStatusUpdate(const Checkable::Ptr& checkable);
	void SendStateChange(const ConfigObject::Ptr& object, const CheckResult::Ptr& cr, StateType type);
	void AddObjectDataToRuntimeUpdates(std::vector<Dictionary::Ptr>& runtimeUpdates, const String& objectKey,
			const String& redisKey, const Dictionary::Ptr& data);

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

	std::vector<String> UpdateObjectAttrs(const ConfigObject::Ptr& object, int fieldType, const String& typeNameOverride);
	Dictionary::Ptr SerializeState(const Checkable::Ptr& checkable);

	/* Stats */
	Dictionary::Ptr GetStats();

	/* utilities */
	static String FormatCheckSumBinary(const String& str);
	static String FormatCommandLine(const Value& commandLine);
	static long long TimestampToMilliseconds(double timestamp);
	static String IcingaToStreamValue(const Value& value);

	static ArrayData GetObjectIdentifiersWithoutEnv(const ConfigObject::Ptr& object);
	static String GetObjectIdentifier(const ConfigObject::Ptr& object);
	static String CalcEventID(const char* eventType, const ConfigObject::Ptr& object, double eventTime = 0, NotificationType nt = NotificationType(0));
	static String GetEnvironment();
	static Dictionary::Ptr SerializeVars(const CustomVarObject::Ptr& object);
	static const char* GetNotificationTypeByEnum(NotificationType type);

	static String HashValue(const Value& value);
	static String HashValue(const Value& value, const std::set<String>& propertiesBlacklist, bool propertiesWhitelist = false);

	static String GetLowerCaseTypeNameDB(const ConfigObject::Ptr& obj);
	static bool PrepareObject(const ConfigObject::Ptr& object, Dictionary::Ptr& attributes, Dictionary::Ptr& checkSums);

	static void StateChangeHandler(const ConfigObject::Ptr& object);
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
	static void AcknowledgementSetHandler(const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type, bool persistent, double changeTime, double expiry);
	static void AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const String& removedBy, double changeTime);

	void AssertOnWorkQueue();

	void ExceptionHandler(boost::exception_ptr exp);

	template<class T>
	static inline
	std::vector<T> Prepend(std::vector<T>&& needle, std::vector<T>&& haystack)
	{
		for (auto& hay : haystack) {
			needle.emplace_back(std::move(hay));
		}

		return std::move(needle);
	}

	template<class T, class Needle>
	static inline
	std::vector<T> Prepend(Needle&& needle, std::vector<T>&& haystack)
	{
		haystack.emplace(haystack.begin(), std::forward<Needle>(needle));
		return std::move(haystack);
	}

	static std::vector<Type::Ptr> GetTypes();

	Timer::Ptr m_StatsTimer;
	WorkQueue m_WorkQueue{0, 1, LogNotice};

	String m_PrefixConfigObject;
	String m_PrefixConfigCheckSum;

	bool m_ConfigDumpInProgress;
	bool m_ConfigDumpDone;

	RedisConnection::Ptr m_Rcon;
	std::unordered_map<ConfigType*, RedisConnection::Ptr> m_Rcons;
	std::atomic_size_t m_PendingRcons;

	struct {
		DumpedGlobals CustomVar, ActionUrl, NotesUrl, IconImage;
	} m_DumpedGlobals;

	static String m_EnvironmentId;
	static boost::once_flag m_EnvironmentIdOnce;
};
}

#endif /* ICINGADB_H */
