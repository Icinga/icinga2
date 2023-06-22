/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CHECKABLE_H
#define CHECKABLE_H

#include "base/atomic.hpp"
#include "base/timer.hpp"
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

	void AddGroup(const String& name);

	bool IsReachable(DependencyType dt = DependencyState, intrusive_ptr<Dependency> *failedDependency = nullptr, int rstack = 0) const;

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

	static Atomic<uint_fast64_t> CurrentConcurrentChecks;

	/* Downtimes */
	int GetDowntimeDepth() const final;

	void RemoveAllDowntimes();
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
	void AddDependency(const intrusive_ptr<Dependency>& dep);
	void RemoveDependency(const intrusive_ptr<Dependency>& dep);
	std::vector<intrusive_ptr<Dependency> > GetDependencies() const;

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
	std::set<intrusive_ptr<Dependency> > m_Dependencies;
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
