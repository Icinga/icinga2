/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CHECKABLE_H
#define CHECKABLE_H

#include "base/atomic.hpp"
#include "base/timer.hpp"
#include "base/shared-object.hpp"
#include "base/process.hpp"
#include "icinga/i2-icinga.hpp"
#include "icinga/checkable-ti.hpp"
#include "icinga/timeperiod.hpp"
#include "icinga/notification.hpp"
#include "icinga/comment.hpp"
#include "icinga/downtime.hpp"
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <limits>
#include <tuple>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key.hpp>

namespace icinga
{

/**
 * @ingroup icinga
 */
enum DependencyType
{
	DependencyState,
	DependencyCheckExecution,
	DependencyNotification
};

/**
 * Checkable Types
 *
 * @ingroup icinga
 */
enum CheckableType
{
	CheckableHost,
	CheckableService
};

/**
 * @ingroup icinga
 */
enum FlappingStateFilter
{
	FlappingStateFilterOk = 1,
	FlappingStateFilterWarning = 2,
	FlappingStateFilterCritical = 4,
	FlappingStateFilterUnknown = 8,
};

class CheckCommand;
class EventCommand;
class Dependency;
class Checkable;

/**
 * A dependency redundancy group that encapsulates its members and other internal logic used by Icinga DB.
 *
 * @ingroup base
 */
class RedundancyGroup : public SharedObject
{
public:
	DECLARE_PTR_TYPEDEFS(RedundancyGroup);

	/**
	 * Defines the key type of each redundancy group members.
	 *
	 * For non-default redundancy groups, that tuple consists of the dependency parent name,
	 * the dependency time period name (empty if not configured), the state filter, and the
	 * ignore soft states flag.
	 *
	 * For the default non-redundant group of a given Checkable, the tuple consists of the
	 * default redundancy group name (which is a randomly generated unique UUID), the child
	 * Checkable name, the parent Checkable name, and the ignore soft states flag (is always false).
	 */
	using MemberTuple = std::tuple<String, String, int, bool>;
	using MemberValueType = std::unordered_multimap<const Checkable*, Dependency*>;
	using MembersMap = std::map<MemberTuple, MemberValueType>;

	RedundancyGroup(String name, const intrusive_ptr<Dependency>& member);

	static void Register(const intrusive_ptr<Dependency>& dependency);
	static void Unregister(const intrusive_ptr<Dependency>& dependency);
	static size_t GetRegistrySize();

	static bool IsDefault(const String& name);
	static MemberTuple MakeCompositeKeyFor(const intrusive_ptr<Dependency>& dep);

	bool IsDefault() const;
	bool HasMembers() const;
	bool HasMembers(const intrusive_ptr<Checkable>& child) const;
	MembersMap GetMembers() const;
	std::set<intrusive_ptr<Dependency>> GetMembers(const Checkable* child) const;
	size_t GetMemberCount() const;

	void SetIcingaDBIdentifier(const String& identifier);
	String GetIcingaDBIdentifier() const;

	const String& GetName() const;
	String GetCompositeKey() const;

	enum State
	{
		Unknown           = 1ull << 0,
		Failed            = 1ull << 1,
		Unreachable       = 1ull << 2,
		ReachableAndOK    = 1ull << 3,
		UnreachableFailed = Unreachable | Failed,
	};

	State GetState(DependencyType dt = DependencyState) const;

protected:
	void AddMember(const intrusive_ptr<Dependency>& member);
	void RemoveMember(const intrusive_ptr<Dependency>& member);
	void MoveMembersTo(const RedundancyGroup::Ptr& other);

	static void RegisterRedundancyGroup(const RedundancyGroup::Ptr& redundancyGroup);
	static void RefreshRegistry(const intrusive_ptr<Dependency>& dependency, const RedundancyGroup::Ptr& newGroup = nullptr);

private:
	mutable std::mutex m_Mutex;
	String m_IcingaDBIdentifier;
	String m_Name;
	MembersMap m_Members;

	struct Hash {
		/**
		 * Calculates the hash value of a redundancy group used by RedundancyGroup::RegistryType.
		 *
		 * @param redundancyGroup The redundancy group to calculate the hash value for.
		 *
		 * @return Returns the hash value of the redundancy group.
		 */
		size_t operator()(const RedundancyGroup::Ptr& redundancyGroup) const
		{
			return boost::hash<std::string>{}(redundancyGroup->GetCompositeKey());
		}
	};

	struct Equal {
		/**
		 * Checks two redundancy groups for equality.
		 *
		 * The equality of two redundancy groups is determined by the equality of their composite keys.
		 * That composite key consists of a tuple of the parent name, the time period name (empty if not configured),
		 * state filter, and the ignore soft states flag of the member.
		 *
		 * @param lhs The first redundancy group to compare.
		 * @param rhs The second redundancy group to compare.
		 *
		 * @return Returns true if the composite keys of the two redundancy groups are equal.
		 */
		bool operator()(const RedundancyGroup::Ptr& lhs, const RedundancyGroup::Ptr& rhs) const
		{
			return lhs->GetCompositeKey() == rhs->GetCompositeKey();
		}
	};

	using RegistryType = boost::multi_index_container<
		RedundancyGroup*, // The type of the elements stored in the container.
		boost::multi_index::indexed_by<
			// The first index is a unique index based on the identity of the redundancy group.
			// The identity of the redundancy group is determined by the provided Hash and Equal functors.
			boost::multi_index::hashed_unique<boost::multi_index::identity<RedundancyGroup*>, Hash, Equal>,
			// This non-unique index allows to search for redundancy groups by their name, and reduces the overall
			// runtime complexity. Without this index, we would have to iterate over all elements to find the on with
			// the desired members and since std::unordered_set doesn't allow erasing elements while iterating, we would
			// have to copy each of them to a temporary container, and then erase and reinsert them back t the original
			// container. This produces way too much overhead, and slows down the startup time of Icinga 2 significantly.
			boost::multi_index::hashed_non_unique<boost::multi_index::key<&RedundancyGroup::GetName>, std::hash<String>>
		>
	>;

	// The global registry of redundancy groups.
	static std::mutex m_RegistryMutex;
	static RegistryType m_Registry;
};

/**
 * An Icinga service.
 *
 * @ingroup icinga
 */
class Checkable : public ObjectImpl<Checkable>
{
public:
	DECLARE_OBJECT(Checkable);
	DECLARE_OBJECTNAME(Checkable);

	static void StaticInitialize();
	static thread_local std::function<void(const Value& commandLine, const ProcessResult&)> ExecuteCommandProcessFinishedHandler;

	Checkable();

	std::set<Checkable::Ptr> GetParents() const;
	std::set<Checkable::Ptr> GetChildren() const;
	std::set<Checkable::Ptr> GetAllChildren() const;
	int GetAllChildrenCount(int rstack = 0) const;

	void AddGroup(const String& name);

	bool IsReachable(DependencyType dt = DependencyState, int rstack = 0) const;
	bool AffectsChildren() const;

	AcknowledgementType GetAcknowledgement();

	void AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, bool notify = true, bool persistent = false, double changeTime = Utility::GetTime(), double expiry = 0, const MessageOrigin::Ptr& origin = nullptr);
	void ClearAcknowledgement(const String& removedBy, double changeTime = Utility::GetTime(), const MessageOrigin::Ptr& origin = nullptr);

	int GetSeverity() const override;
	bool GetProblem() const override;
	bool GetHandled() const override;
	Timestamp GetNextUpdate() const override;

	/* Checks */
	intrusive_ptr<CheckCommand> GetCheckCommand() const;
	TimePeriod::Ptr GetCheckPeriod() const;

	long GetSchedulingOffset();
	void SetSchedulingOffset(long offset);

	void UpdateNextCheck(const MessageOrigin::Ptr& origin = nullptr);

	bool HasBeenChecked() const;
	virtual bool IsStateOK(ServiceState state) const = 0;

	double GetLastCheck() const final;

	virtual void SaveLastState(ServiceState state, double timestamp) = 0;

	static void UpdateStatistics(const CheckResult::Ptr& cr, CheckableType type);

	void ExecuteRemoteCheck(const Dictionary::Ptr& resolvedMacros = nullptr);
	void ExecuteCheck();
	enum class ProcessingResult
	{
		Ok,
		NoCheckResult,
		CheckableInactive,
		NewerCheckResultPresent,
	};
	ProcessingResult ProcessCheckResult(const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin = nullptr);

	Endpoint::Ptr GetCommandEndpoint() const;

	static boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, const MessageOrigin::Ptr&)> OnNewCheckResult;
	static boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, StateType, const MessageOrigin::Ptr&)> OnStateChange;
	static boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, std::set<Checkable::Ptr>, const MessageOrigin::Ptr&)> OnReachabilityChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, NotificationType, const CheckResult::Ptr&,
		const String&, const String&, const MessageOrigin::Ptr&)> OnNotificationsRequested;
	static boost::signals2::signal<void (const Notification::Ptr&, const Checkable::Ptr&, const User::Ptr&,
		const NotificationType&, const CheckResult::Ptr&, const String&, const String&, const String&,
		const MessageOrigin::Ptr&)> OnNotificationSentToUser;
	static boost::signals2::signal<void (const Notification::Ptr&, const Checkable::Ptr&, const std::set<User::Ptr>&,
		const NotificationType&, const CheckResult::Ptr&, const String&,
		const String&, const MessageOrigin::Ptr&)> OnNotificationSentToAllUsers;
	static boost::signals2::signal<void (const Checkable::Ptr&, const String&, const String&, AcknowledgementType,
		bool, bool, double, double, const MessageOrigin::Ptr&)> OnAcknowledgementSet;
	static boost::signals2::signal<void (const Checkable::Ptr&, const String&, double, const MessageOrigin::Ptr&)> OnAcknowledgementCleared;
	static boost::signals2::signal<void (const Checkable::Ptr&, double)> OnFlappingChange;
	static boost::signals2::signal<void (const Checkable::Ptr&)> OnNextCheckUpdated;
	static boost::signals2::signal<void (const Checkable::Ptr&)> OnEventCommandExecuted;

	static boost::signals2::signal<void (const intrusive_ptr<Dependency>&, const std::vector<RedundancyGroup::Ptr>&)> OnRedundancyGroupsChanged;

	static Atomic<uint_fast64_t> CurrentConcurrentChecks;

	/* Downtimes */
	int GetDowntimeDepth() const final;

	void TriggerDowntimes(double triggerTime);
	bool IsInDowntime() const;
	bool IsAcknowledged() const;

	std::set<Downtime::Ptr> GetDowntimes() const;
	void RegisterDowntime(const Downtime::Ptr& downtime);
	void UnregisterDowntime(const Downtime::Ptr& downtime);

	/* Comments */
	void RemoveAllComments();
	void RemoveAckComments(const String& removedBy = String(), double createdBefore = std::numeric_limits<double>::max());

	std::set<Comment::Ptr> GetComments() const;
	Comment::Ptr GetLastComment() const;
	void RegisterComment(const Comment::Ptr& comment);
	void UnregisterComment(const Comment::Ptr& comment);

	/* Notifications */
	void SendNotifications(NotificationType type, const CheckResult::Ptr& cr, const String& author = "", const String& text = "");

	std::set<Notification::Ptr> GetNotifications() const;
	void RegisterNotification(const Notification::Ptr& notification);
	void UnregisterNotification(const Notification::Ptr& notification);

	void ResetNotificationNumbers();

	/* Event Handler */
	void ExecuteEventHandler(const Dictionary::Ptr& resolvedMacros = nullptr,
		bool useResolvedMacros = false);

	intrusive_ptr<EventCommand> GetEventCommand() const;

	/* Flapping Detection */
	bool IsFlapping() const;

	/* Dependencies */
	void AddDependency(const intrusive_ptr<Dependency>& dep) const;
	void AddRedundancyGroup(const RedundancyGroup::Ptr& redundancyGroup);
	void RemoveDependency(const intrusive_ptr<Dependency>& dep) const;
	void RemoveRedundancyGroup(const RedundancyGroup::Ptr& redundancyGroup);
	std::vector<intrusive_ptr<Dependency> > GetDependencies() const;
	std::vector<RedundancyGroup::Ptr> GetRedundancyGroups() const;

	void AddReverseDependency(const intrusive_ptr<Dependency>& dep);
	void RemoveReverseDependency(const intrusive_ptr<Dependency>& dep);
	std::vector<intrusive_ptr<Dependency> > GetReverseDependencies() const;

	void ValidateCheckInterval(const Lazy<double>& lvalue, const ValidationUtils& value) final;
	void ValidateRetryInterval(const Lazy<double>& lvalue, const ValidationUtils& value) final;
	void ValidateMaxCheckAttempts(const Lazy<int>& lvalue, const ValidationUtils& value) final;

	bool NotificationReasonApplies(NotificationType type);
	bool NotificationReasonSuppressed(NotificationType type);
	bool IsLikelyToBeCheckedSoon();

	void FireSuppressedNotifications();

	static void IncreasePendingChecks();
	static void DecreasePendingChecks();
	static int GetPendingChecks();
	static void AquirePendingCheckSlot(int maxPendingChecks);

	static Object::Ptr GetPrototype();

protected:
	void Start(bool runtimeCreated) override;
	void OnConfigLoaded() override;
	void OnAllConfigLoaded() override;

private:
	mutable std::mutex m_CheckableMutex;
	bool m_CheckRunning{false};
	long m_SchedulingOffset;

	static std::mutex m_StatsMutex;
	static int m_PendingChecks;
	static std::condition_variable m_PendingChecksCV;

	/* Downtimes */
	std::set<Downtime::Ptr> m_Downtimes;
	mutable std::mutex m_DowntimeMutex;

	static void NotifyFixedDowntimeStart(const Downtime::Ptr& downtime);
	static void NotifyFlexibleDowntimeStart(const Downtime::Ptr& downtime);
	static void NotifyDowntimeInternal(const Downtime::Ptr& downtime);

	static void NotifyDowntimeEnd(const Downtime::Ptr& downtime);

	static void FireSuppressedNotificationsTimer(const Timer * const&);
	static void CleanDeadlinedExecutions(const Timer * const&);

	/* Comments */
	std::set<Comment::Ptr> m_Comments;
	mutable std::mutex m_CommentMutex;

	/* Notifications */
	std::set<Notification::Ptr> m_Notifications;
	mutable std::mutex m_NotificationMutex;

	/* Dependencies */
	mutable std::mutex m_DependencyMutex;
	std::set<RedundancyGroup::Ptr> m_Dependencies;
	std::set<intrusive_ptr<Dependency> > m_ReverseDependencies;

	void GetAllChildrenInternal(std::set<Checkable::Ptr>& children, int level = 0) const;

	/* Flapping */
	static const std::map<String, int> m_FlappingStateFilterMap;

	void UpdateFlappingStatus(ServiceState newState);
	static int ServiceStateToFlappingFilter(ServiceState state);
};

}

#endif /* CHECKABLE_H */

#include "icinga/dependency.hpp"
