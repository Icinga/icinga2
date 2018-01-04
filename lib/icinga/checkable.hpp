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

#ifndef CHECKABLE_H
#define CHECKABLE_H

#include "icinga/i2-icinga.hpp"
#include "icinga/checkable.thpp"
#include "icinga/timeperiod.hpp"
#include "icinga/notification.hpp"
#include "icinga/comment.hpp"
#include "icinga/downtime.hpp"
#include "remote/endpoint.hpp"
#include "remote/messageorigin.hpp"

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
 * Severity Flags
 *
 * @ingroup icinga
 */
enum SeverityFlag
{
	SeverityFlagDowntime = 1,
	SeverityFlagAcknowledgement = 2,
	SeverityFlagUnhandled = 8,
	SeverityFlagPending = 16,
	SeverityFlagWarning = 32,
	SeverityFlagUnknown = 64,
	SeverityFlagCritical = 128,
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

	Checkable();

	std::set<Checkable::Ptr> GetParents() const;
	std::set<Checkable::Ptr> GetChildren() const;
	std::set<Checkable::Ptr> GetAllChildren() const;

	void AddGroup(const String& name);

	bool IsReachable(DependencyType dt = DependencyState, intrusive_ptr<Dependency> *failedDependency = nullptr, int rstack = 0) const;

	AcknowledgementType GetAcknowledgement();

	void AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, bool notify = true, bool persistent = false, double expiry = 0, const MessageOrigin::Ptr& origin = nullptr);
	void ClearAcknowledgement(const MessageOrigin::Ptr& origin = nullptr);

	virtual int GetSeverity() const override;

	/* Checks */
	intrusive_ptr<CheckCommand> GetCheckCommand() const;
	TimePeriod::Ptr GetCheckPeriod() const;

	long GetSchedulingOffset();
	void SetSchedulingOffset(long offset);

	void UpdateNextCheck(const MessageOrigin::Ptr& origin = nullptr);

	bool HasBeenChecked() const;
	virtual bool IsStateOK(ServiceState state) = 0;

	double GetLastCheck(void) const final;

	virtual void SaveLastState(ServiceState state, double timestamp) = 0;

	static void UpdateStatistics(const CheckResult::Ptr& cr, CheckableType type);

	void ExecuteRemoteCheck(const Dictionary::Ptr& resolvedMacros = nullptr);
	void ExecuteCheck();
	void ProcessCheckResult(const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin = nullptr);

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
		bool, bool, double, const MessageOrigin::Ptr&)> OnAcknowledgementSet;
	static boost::signals2::signal<void (const Checkable::Ptr&, const MessageOrigin::Ptr&)> OnAcknowledgementCleared;
	static boost::signals2::signal<void (const Checkable::Ptr&)> OnNextCheckUpdated;
	static boost::signals2::signal<void (const Checkable::Ptr&)> OnEventCommandExecuted;

	/* Downtimes */
	int GetDowntimeDepth(void) const final;

	void RemoveAllDowntimes();
	void TriggerDowntimes();
	bool IsInDowntime() const;
	bool IsAcknowledged() const;

	std::set<Downtime::Ptr> GetDowntimes() const;
	void RegisterDowntime(const Downtime::Ptr& downtime);
	void UnregisterDowntime(const Downtime::Ptr& downtime);

	/* Comments */
	void RemoveAllComments();
	void RemoveCommentsByType(int type);

	std::set<Comment::Ptr> GetComments() const;
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

	virtual void ValidateCheckInterval(double value, const ValidationUtils& utils) override final;
	virtual void ValidateMaxCheckAttempts(int value, const ValidationUtils& utils) override final;

	static void IncreasePendingChecks();
	static void DecreasePendingChecks();
	static int GetPendingChecks();

	static Object::Ptr GetPrototype();

protected:
	virtual void Start(bool runtimeCreated) override;
	virtual void OnAllConfigLoaded() override;

private:
	mutable boost::mutex m_CheckableMutex;
	bool m_CheckRunning;
	long m_SchedulingOffset;

	static boost::mutex m_StatsMutex;
	static int m_PendingChecks;

	/* Downtimes */
	std::set<Downtime::Ptr> m_Downtimes;
	mutable boost::mutex m_DowntimeMutex;

	static void NotifyFixedDowntimeStart(const Downtime::Ptr& downtime);
	static void NotifyFlexibleDowntimeStart(const Downtime::Ptr& downtime);
	static void NotifyDowntimeInternal(const Downtime::Ptr& downtime);

	static void NotifyDowntimeEnd(const Downtime::Ptr& downtime);

	/* Comments */
	std::set<Comment::Ptr> m_Comments;
	mutable boost::mutex m_CommentMutex;

	/* Notifications */
	std::set<Notification::Ptr> m_Notifications;
	mutable boost::mutex m_NotificationMutex;

	/* Dependencies */
	mutable boost::mutex m_DependencyMutex;
	std::set<intrusive_ptr<Dependency> > m_Dependencies;
	std::set<intrusive_ptr<Dependency> > m_ReverseDependencies;

	void GetAllChildrenInternal(std::set<Checkable::Ptr>& children, int level = 0) const;

	/* Flapping */
	void UpdateFlappingStatus(bool stateChange);
};

}

#endif /* CHECKABLE_H */

#include "icinga/dependency.hpp"
