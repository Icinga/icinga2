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

#ifndef CHECKABLE_H
#define CHECKABLE_H

#include "icinga/i2-icinga.hpp"
#include "icinga/checkable.thpp"
#include "icinga/timeperiod.hpp"
#include "icinga/notification.hpp"
#include "icinga/comment.hpp"
#include "icinga/downtime.hpp"
#include "remote/messageorigin.hpp"

namespace icinga
{

/**
 * The state of service flapping.
 *
 * @ingroup icinga
 */
enum FlappingState
{
	FlappingStarted = 0,
	FlappingDisabled = 1,
	FlappingStopped = 2,
	FlappingEnabled = 3
};

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

class CheckCommand;
class EventCommand;
class Dependency;

/**
 * An Icinga service.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Checkable : public ObjectImpl<Checkable>
{
public:
	DECLARE_PTR_TYPEDEFS(Checkable);
	DECLARE_TYPENAME(Checkable);

	Checkable(void);

	std::set<Checkable::Ptr> GetParents(void) const;
	std::set<Checkable::Ptr> GetChildren(void) const;

	void AddGroup(const String& name);

	//bool IsHostCheck(void) const;

	bool IsReachable(DependencyType dt = DependencyState, shared_ptr<Dependency> *failedDependency = NULL, int rstack = 0) const;

	AcknowledgementType GetAcknowledgement(void);

	void AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, double expiry = 0, const MessageOrigin& origin = MessageOrigin());
	void ClearAcknowledgement(const MessageOrigin& origin = MessageOrigin());

	/* Checks */
	shared_ptr<CheckCommand> GetCheckCommand(void) const;
	void SetCheckCommand(const shared_ptr<CheckCommand>& command);

	TimePeriod::Ptr GetCheckPeriod(void) const;
	void SetCheckPeriod(const TimePeriod::Ptr& tp);

	double GetCheckInterval(void) const;
	void SetCheckInterval(double interval, const MessageOrigin& origin = MessageOrigin());

	double GetRetryInterval(void) const;
	void SetRetryInterval(double interval, const MessageOrigin& origin = MessageOrigin());

	int GetMaxCheckAttempts(void) const;
	void SetMaxCheckAttempts(int attempts);

	long GetSchedulingOffset(void);
	void SetSchedulingOffset(long offset);

	void SetNextCheck(double nextCheck, const MessageOrigin& origin = MessageOrigin());
	double GetNextCheck(void);
	void UpdateNextCheck(void);

	bool HasBeenChecked(void) const;

	double GetLastCheck(void) const;

	bool GetEnableActiveChecks(void) const;
	void SetEnableActiveChecks(bool enabled, const MessageOrigin& origin = MessageOrigin());

	bool GetEnablePassiveChecks(void) const;
	void SetEnablePassiveChecks(bool enabled, const MessageOrigin& origin = MessageOrigin());

	bool GetForceNextCheck(void) const;
	void SetForceNextCheck(bool forced, const MessageOrigin& origin = MessageOrigin());

	static void UpdateStatistics(const CheckResult::Ptr& cr, CheckableType type);

	void ExecuteCheck(void);
	void ProcessCheckResult(const CheckResult::Ptr& cr, const MessageOrigin& origin = MessageOrigin());

	int GetModifiedAttributes(void) const;
	void SetModifiedAttributes(int flags, const MessageOrigin& origin = MessageOrigin());

	bool IsCheckPending(void) const;

	static double CalculateExecutionTime(const CheckResult::Ptr& cr);
	static double CalculateLatency(const CheckResult::Ptr& cr);

	static boost::signals2::signal<void (const Checkable::Ptr&, double, const MessageOrigin&)> OnNextCheckChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> OnForceNextCheckChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> OnForceNextNotificationChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> OnEnableActiveChecksChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> OnEnablePassiveChecksChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> OnEnableNotificationsChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> OnEnableFlappingChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> OnEnablePerfdataChanged;

	static boost::signals2::signal<void (const Checkable::Ptr&, bool, const MessageOrigin&)> OnEnableEventHandlerChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, double, const MessageOrigin&)> OnCheckIntervalChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, double, const MessageOrigin&)> OnRetryIntervalChanged;

	static boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, const MessageOrigin&)> OnNewCheckResult;
	static boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, StateType, const MessageOrigin&)> OnStateChange;
	static boost::signals2::signal<void (const Checkable::Ptr&, NotificationType, const CheckResult::Ptr&,
	    const String&, const String&)> OnNotificationsRequested;
	static boost::signals2::signal<void (const Notification::Ptr&, const Checkable::Ptr&, const std::set<User::Ptr>&,
	    const NotificationType&, const CheckResult::Ptr&, const String&,
	    const String&)> OnNotificationSendStart;
	static boost::signals2::signal<void (const Notification::Ptr&, const Checkable::Ptr&, const User::Ptr&,
	    const NotificationType&, const CheckResult::Ptr&, const String&,
	    const String&, const String&)> OnNotificationSentToUser;
	static boost::signals2::signal<void (const Notification::Ptr&, const Checkable::Ptr&, const std::set<User::Ptr>&,
	    const NotificationType&, const CheckResult::Ptr&, const String&,
	    const String&)> OnNotificationSentToAllUsers;
	static boost::signals2::signal<void (const Checkable::Ptr&, const Comment::Ptr&, const MessageOrigin&)> OnCommentAdded;
	static boost::signals2::signal<void (const Checkable::Ptr&, const Comment::Ptr&, const MessageOrigin&)> OnCommentRemoved;
	static boost::signals2::signal<void (const Checkable::Ptr&, const Downtime::Ptr&, const MessageOrigin&)> OnDowntimeAdded;
	static boost::signals2::signal<void (const Checkable::Ptr&, const Downtime::Ptr&, const MessageOrigin&)> OnDowntimeRemoved;
	static boost::signals2::signal<void (const Checkable::Ptr&, FlappingState)> OnFlappingChanged;
	static boost::signals2::signal<void (const Checkable::Ptr&, const Downtime::Ptr&)> OnDowntimeTriggered;
	static boost::signals2::signal<void (const Checkable::Ptr&, const String&, const String&, AcknowledgementType,
					     double, const MessageOrigin&)> OnAcknowledgementSet;
	static boost::signals2::signal<void (const Checkable::Ptr&, const MessageOrigin&)> OnAcknowledgementCleared;
	static boost::signals2::signal<void (const Checkable::Ptr&)> OnEventCommandExecuted;

	/* Downtimes */
	static int GetNextDowntimeID(void);

	int GetDowntimeDepth(void) const;

	String AddDowntime(const String& author, const String& comment,
	    double startTime, double endTime, bool fixed,
	    const String& triggeredBy, double duration,
	    const String& scheduledBy = String(), const String& id = String(),
	    const MessageOrigin& origin = MessageOrigin());

	static void RemoveDowntime(const String& id, bool cancelled, const MessageOrigin& origin = MessageOrigin());

	void TriggerDowntimes(void);
	static void TriggerDowntime(const String& id);

	static String GetDowntimeIDFromLegacyID(int id);
	static Checkable::Ptr GetOwnerByDowntimeID(const String& id);
	static Downtime::Ptr GetDowntimeByID(const String& id);

	static void StartDowntimesExpiredTimer(void);

	bool IsInDowntime(void) const;
	bool IsAcknowledged(void);

	/* Comments */
	static int GetNextCommentID(void);

	String AddComment(CommentType entryType, const String& author,
	    const String& text, double expireTime, const String& id = String(), const MessageOrigin& origin = MessageOrigin());

	void RemoveAllComments(void);
	void RemoveCommentsByType(int type);
	static void RemoveComment(const String& id, const MessageOrigin& origin = MessageOrigin());

	static String GetCommentIDFromLegacyID(int id);
	static Checkable::Ptr GetOwnerByCommentID(const String& id);
	static Comment::Ptr GetCommentByID(const String& id);

	/* Notifications */
	bool GetEnableNotifications(void) const;
	void SetEnableNotifications(bool enabled, const MessageOrigin& origin = MessageOrigin());

	void SendNotifications(NotificationType type, const CheckResult::Ptr& cr, const String& author = "", const String& text = "");

	std::set<Notification::Ptr> GetNotifications(void) const;
	void AddNotification(const Notification::Ptr& notification);
	void RemoveNotification(const Notification::Ptr& notification);

	void SetForceNextNotification(bool force, const MessageOrigin& origin = MessageOrigin());
	bool GetForceNextNotification(void) const;

	void ResetNotificationNumbers(void);

	/* Event Handler */
	void ExecuteEventHandler(void);

	shared_ptr<EventCommand> GetEventCommand(void) const;
	void SetEventCommand(const shared_ptr<EventCommand>& command);

	bool GetEnableEventHandler(void) const;
	void SetEnableEventHandler(bool enabled, const MessageOrigin& origin = MessageOrigin());

	/* Flapping Detection */
	double GetFlappingCurrent(void) const;

	bool GetEnableFlapping(void) const;
	void SetEnableFlapping(bool enabled, const MessageOrigin& origin = MessageOrigin());

	bool IsFlapping(void) const;
	void UpdateFlappingStatus(bool stateChange);

	/* Performance data */
	bool GetEnablePerfdata(void) const;
	void SetEnablePerfdata(bool enabled, const MessageOrigin& origin = MessageOrigin());

	/* Dependencies */
	void AddDependency(const shared_ptr<Dependency>& dep);
	void RemoveDependency(const shared_ptr<Dependency>& dep);
	std::set<shared_ptr<Dependency> > GetDependencies(void) const;

	void AddReverseDependency(const shared_ptr<Dependency>& dep);
	void RemoveReverseDependency(const shared_ptr<Dependency>& dep);
	std::set<shared_ptr<Dependency> > GetReverseDependencies(void) const;

protected:
	virtual void Start(void);

	virtual void OnConfigLoaded(void);
	virtual void OnStateLoaded(void);

private:
	mutable boost::mutex m_CheckableMutex;
	bool m_CheckRunning;
	long m_SchedulingOffset;

	/* Downtimes */
	static void DowntimesExpireTimerHandler(void);
	void RemoveExpiredDowntimes(void);
	void AddDowntimesToCache(void);

	/* Comments */
	static void CommentsExpireTimerHandler(void);
	void RemoveExpiredComments(void);
	void AddCommentsToCache(void);

	/* Notifications */
	std::set<Notification::Ptr> m_Notifications;

	/* Dependencies */
	mutable boost::mutex m_DependencyMutex;
	std::set<shared_ptr<Dependency> > m_Dependencies;
	std::set<shared_ptr<Dependency> > m_ReverseDependencies;
};

}

#endif /* CHECKABLE_H */
