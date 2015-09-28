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
	DECLARE_OBJECT(Checkable);
	DECLARE_OBJECTNAME(Checkable);

	Checkable(void);

	std::set<Checkable::Ptr> GetParents(void) const;
	std::set<Checkable::Ptr> GetChildren(void) const;

	void AddGroup(const String& name);

	//bool IsHostCheck(void) const;

	bool IsReachable(DependencyType dt = DependencyState, intrusive_ptr<Dependency> *failedDependency = NULL, int rstack = 0) const;

	AcknowledgementType GetAcknowledgement(void);

	void AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, bool notify = true, double expiry = 0, const MessageOrigin::Ptr& origin = MessageOrigin::Ptr());
	void ClearAcknowledgement(const MessageOrigin::Ptr& origin = MessageOrigin::Ptr());

	/* Checks */
	intrusive_ptr<CheckCommand> GetCheckCommand(void) const;
	TimePeriod::Ptr GetCheckPeriod(void) const;

	long GetSchedulingOffset(void);
	void SetSchedulingOffset(long offset);

	void UpdateNextCheck(void);

	bool HasBeenChecked(void) const;

	double GetLastCheck(void) const;

	static void UpdateStatistics(const CheckResult::Ptr& cr, CheckableType type);

	void ExecuteRemoteCheck(const Dictionary::Ptr& resolvedMacros = Dictionary::Ptr());
	void ExecuteCheck();
	void ProcessCheckResult(const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin = MessageOrigin::Ptr());

	Endpoint::Ptr GetCommandEndpoint(void) const;

	bool IsCheckPending(void) const;

	static double CalculateExecutionTime(const CheckResult::Ptr& cr);
	static double CalculateLatency(const CheckResult::Ptr& cr);

	static boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, const MessageOrigin::Ptr&)> OnNewCheckResult;
	static boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, StateType, const MessageOrigin::Ptr&)> OnStateChange;
	static boost::signals2::signal<void (const Checkable::Ptr&, const CheckResult::Ptr&, std::set<Checkable::Ptr>, const MessageOrigin::Ptr&)> OnReachabilityChanged;
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
	static boost::signals2::signal<void (const Checkable::Ptr&, const Comment::Ptr&, const MessageOrigin::Ptr&)> OnCommentAdded;
	static boost::signals2::signal<void (const Checkable::Ptr&, const Comment::Ptr&, const MessageOrigin::Ptr&)> OnCommentRemoved;
	static boost::signals2::signal<void (const Checkable::Ptr&, const Downtime::Ptr&, const MessageOrigin::Ptr&)> OnDowntimeAdded;
	static boost::signals2::signal<void (const Checkable::Ptr&, const Downtime::Ptr&, const MessageOrigin::Ptr&)> OnDowntimeRemoved;
	static boost::signals2::signal<void (const Checkable::Ptr&, const Downtime::Ptr&)> OnDowntimeTriggered;
	static boost::signals2::signal<void (const Checkable::Ptr&, const String&, const String&, AcknowledgementType,
					     bool, double, const MessageOrigin::Ptr&)> OnAcknowledgementSet;
	static boost::signals2::signal<void (const Checkable::Ptr&, const MessageOrigin::Ptr&)> OnAcknowledgementCleared;
	static boost::signals2::signal<void (const Checkable::Ptr&)> OnEventCommandExecuted;

	/* Downtimes */
	static int GetNextDowntimeID(void);

	int GetDowntimeDepth(void) const;

	String AddDowntime(const String& author, const String& comment,
	    double startTime, double endTime, bool fixed,
	    const String& triggeredBy, double duration,
	    const String& scheduledBy = String(), const String& id = String(),
	    const MessageOrigin::Ptr& origin = MessageOrigin::Ptr());

	void RemoveAllDowntimes(void);
	static void RemoveDowntime(const String& id, bool cancelled, const MessageOrigin::Ptr& origin = MessageOrigin::Ptr());

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
	    const String& text, double expireTime, const String& id = String(), const MessageOrigin::Ptr& origin = MessageOrigin::Ptr());

	void RemoveAllComments(void);
	void RemoveCommentsByType(int type);
	static void RemoveComment(const String& id, const MessageOrigin::Ptr& origin = MessageOrigin::Ptr());

	static String GetCommentIDFromLegacyID(int id);
	static Checkable::Ptr GetOwnerByCommentID(const String& id);
	static Comment::Ptr GetCommentByID(const String& id);

	/* Notifications */
	void SendNotifications(NotificationType type, const CheckResult::Ptr& cr, const String& author = "", const String& text = "");

	std::set<Notification::Ptr> GetNotifications(void) const;
	void AddNotification(const Notification::Ptr& notification);
	void RemoveNotification(const Notification::Ptr& notification);

	void ResetNotificationNumbers(void);

	/* Event Handler */
	void ExecuteEventHandler(const Dictionary::Ptr& resolvedMacros = Dictionary::Ptr(),
	    bool useResolvedMacros = false);

	intrusive_ptr<EventCommand> GetEventCommand(void) const;

	/* Flapping Detection */
	double GetFlappingCurrent(void) const;

	bool IsFlapping(void) const;
	void UpdateFlappingStatus(bool stateChange);

	/* Dependencies */
	void AddDependency(const intrusive_ptr<Dependency>& dep);
	void RemoveDependency(const intrusive_ptr<Dependency>& dep);
	std::set<intrusive_ptr<Dependency> > GetDependencies(void) const;

	void AddReverseDependency(const intrusive_ptr<Dependency>& dep);
	void RemoveReverseDependency(const intrusive_ptr<Dependency>& dep);
	std::set<intrusive_ptr<Dependency> > GetReverseDependencies(void) const;

	virtual void ValidateCheckInterval(double value, const ValidationUtils& utils) override;

protected:
	virtual void Start(void) override;

	virtual void OnStateLoaded(void) override;

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
	mutable boost::mutex m_NotificationMutex;

	/* Dependencies */
	mutable boost::mutex m_DependencyMutex;
	std::set<intrusive_ptr<Dependency> > m_Dependencies;
	std::set<intrusive_ptr<Dependency> > m_ReverseDependencies;
};

}

#endif /* CHECKABLE_H */

#include "icinga/dependency.hpp"
