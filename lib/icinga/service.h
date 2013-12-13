/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#ifndef SERVICE_H
#define SERVICE_H

#include "icinga/i2-icinga.h"
#include "icinga/service.th"
#include "icinga/macroresolver.h"
#include "icinga/host.h"
#include "icinga/timeperiod.h"
#include "icinga/notification.h"
#include "icinga/comment.h"
#include "icinga/downtime.h"
#include "base/i2-base.h"
#include "base/array.h"
#include <boost/signals2.hpp>
#include <boost/thread/once.hpp>

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
 * Modified attributes.
 *
 * @ingroup icinga
 */
enum ModifiedAttributeType
{
	ModAttrNotificationsEnabled = 1,
	ModAttrActiveChecksEnabled = 2,
	ModAttrPassiveChecksEnabled = 4,
	ModAttrEventHandlerEnabled = 8,
	ModAttrFlapDetectionEnabled = 16,
	ModAttrFailurePredictionEnabled = 32,
	ModAttrPerformanceDataEnabled = 64,
	ModAttrObsessiveHandlerEnabled = 128,
	ModAttrEventHandlerCommand = 256,
	ModAttrCheckCommand = 512,
	ModAttrNormalCheckInterval = 1024,
	ModAttrRetryCheckInterval = 2048,
	ModAttrMaxCheckAttempts = 4096,
	ModAttrFreshnessChecksEnabled = 8192,
	ModAttrCheckTimeperiod = 16384,
	ModAttrCustomVariable = 32768,
	ModAttrNotificationTimeperiod = 65536
};

class CheckCommand;
class EventCommand;

/**
 * An Icinga service.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Service : public ObjectImpl<Service>, public MacroResolver
{
public:
	DECLARE_PTR_TYPEDEFS(Service);
	DECLARE_TYPENAME(Service);

	Service(void);

	static Service::Ptr GetByNamePair(const String& hostName, const String& serviceName);

	Host::Ptr GetHost(void) const;

	std::set<Host::Ptr> GetParentHosts(void) const;
	std::set<Service::Ptr> GetParentServices(void) const;

	bool IsHostCheck(void) const;

	bool IsReachable(void) const;

	AcknowledgementType GetAcknowledgement(void);

	void AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, double expiry = 0, const String& authority = String());
	void ClearAcknowledgement(const String& authority = String());

	/* Checks */
	shared_ptr<CheckCommand> GetCheckCommand(void) const;
	void SetCheckCommand(const shared_ptr<CheckCommand>& command);

	TimePeriod::Ptr GetCheckPeriod(void) const;
	void SetCheckPeriod(const TimePeriod::Ptr& tp);

	double GetCheckInterval(void) const;
	void SetCheckInterval(double interval);

	double GetRetryInterval(void) const;
	void SetRetryInterval(double interval);

	int GetMaxCheckAttempts(void) const;
	void SetMaxCheckAttempts(int attempts);

	long GetSchedulingOffset(void);
	void SetSchedulingOffset(long offset);

	void SetNextCheck(double nextCheck, const String& authority = String());
	double GetNextCheck(void);
	void UpdateNextCheck(void);

	bool HasBeenChecked(void) const;

	double GetLastCheck(void) const;

	bool GetEnableActiveChecks(void) const;
	void SetEnableActiveChecks(bool enabled, const String& authority = String());

	bool GetEnablePassiveChecks(void) const;
	void SetEnablePassiveChecks(bool enabled, const String& authority = String());

	bool GetForceNextCheck(void) const;
	void SetForceNextCheck(bool forced, const String& authority = String());

	static void UpdateStatistics(const CheckResult::Ptr& cr);

	void ExecuteCheck(void);
	void ProcessCheckResult(const CheckResult::Ptr& cr, const String& authority = String());

	int GetModifiedAttributes(void) const;
	void SetModifiedAttributes(int flags);

	static double CalculateExecutionTime(const CheckResult::Ptr& cr);
	static double CalculateLatency(const CheckResult::Ptr& cr);

	static ServiceState StateFromString(const String& state);
	static String StateToString(ServiceState state);

	static StateType StateTypeFromString(const String& state);
	static String StateTypeToString(StateType state);

	static boost::signals2::signal<void (const Service::Ptr&, double, const String&)> OnNextCheckChanged;
	static boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> OnForceNextCheckChanged;
	static boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> OnForceNextNotificationChanged;
	static boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> OnEnableActiveChecksChanged;
	static boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> OnEnablePassiveChecksChanged;
	static boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> OnEnableNotificationsChanged;
	static boost::signals2::signal<void (const Service::Ptr&, bool, const String&)> OnEnableFlappingChanged;
	static boost::signals2::signal<void (const Service::Ptr&, const CheckResult::Ptr&, const String&)> OnNewCheckResult;
	static boost::signals2::signal<void (const Service::Ptr&, const CheckResult::Ptr&, StateType, const String&)> OnStateChange;
	static boost::signals2::signal<void (const Service::Ptr&, NotificationType, const CheckResult::Ptr&, const String&, const String&)> OnNotificationsRequested;
	static boost::signals2::signal<void (const Service::Ptr&, const User::Ptr&, const NotificationType&, const CheckResult::Ptr&, const String&, const String&, const String&)> OnNotificationSentToUser;
	static boost::signals2::signal<void (const Service::Ptr&, const std::set<User::Ptr>&, const NotificationType&, const CheckResult::Ptr&, const String&, const String&)> OnNotificationSentToAllUsers;
	static boost::signals2::signal<void (const Service::Ptr&, const Comment::Ptr&, const String&)> OnCommentAdded;
	static boost::signals2::signal<void (const Service::Ptr&, const Comment::Ptr&, const String&)> OnCommentRemoved;
	static boost::signals2::signal<void (const Service::Ptr&, const Downtime::Ptr&, const String&)> OnDowntimeAdded;
	static boost::signals2::signal<void (const Service::Ptr&, const Downtime::Ptr&, const String&)> OnDowntimeRemoved;
	static boost::signals2::signal<void (const Service::Ptr&, FlappingState)> OnFlappingChanged;
	static boost::signals2::signal<void (const Service::Ptr&, const Downtime::Ptr&)> OnDowntimeTriggered;
	static boost::signals2::signal<void (const Service::Ptr&, const String&, const String&, AcknowledgementType, double, const String&)> OnAcknowledgementSet;
	static boost::signals2::signal<void (const Service::Ptr&, const String&)> OnAcknowledgementCleared;
	static boost::signals2::signal<void (const Service::Ptr&)> OnEventCommandExecuted;

	virtual bool ResolveMacro(const String& macro, const CheckResult::Ptr& cr, String *result) const;

	/* Downtimes */
	static int GetNextDowntimeID(void);

	int GetDowntimeDepth(void) const;

	String AddDowntime(const String& author, const String& comment,
	    double startTime, double endTime, bool fixed,
	    const String& triggeredBy, double duration,
	    const String& scheduledBy = String(), const String& id = String(),
	    const String& authority = String());

	static void RemoveDowntime(const String& id, bool cancelled, const String& = String());

        void TriggerDowntimes(void);
	static void TriggerDowntime(const String& id);

	static String GetDowntimeIDFromLegacyID(int id);
	static Service::Ptr GetOwnerByDowntimeID(const String& id);
	static Downtime::Ptr GetDowntimeByID(const String& id);

	static void StartDowntimesExpiredTimer(void);

	bool IsInDowntime(void) const;
	bool IsAcknowledged(void);

	void UpdateSlaveScheduledDowntimes(void);

	/* Comments */
	static int GetNextCommentID(void);

	String AddComment(CommentType entryType, const String& author,
	    const String& text, double expireTime, const String& id = String(), const String& authority = String());

	void RemoveAllComments(void);
	void RemoveCommentsByType(int type);
	static void RemoveComment(const String& id, const String& authority = String());

	static String GetCommentIDFromLegacyID(int id);
	static Service::Ptr GetOwnerByCommentID(const String& id);
	static Comment::Ptr GetCommentByID(const String& id);

	/* Notifications */
	bool GetEnableNotifications(void) const;
	void SetEnableNotifications(bool enabled, const String& authority = String());

	void SendNotifications(NotificationType type, const CheckResult::Ptr& cr, const String& author = "", const String& text = "");

	std::set<Notification::Ptr> GetNotifications(void) const;
	void AddNotification(const Notification::Ptr& notification);
	void RemoveNotification(const Notification::Ptr& notification);

	void SetForceNextNotification(bool force, const String& authority = String());
	bool GetForceNextNotification(void) const;

	void ResetNotificationNumbers(void);

	void UpdateSlaveNotifications(void);

	/* Event Handler */
	void ExecuteEventHandler(void);

	shared_ptr<EventCommand> GetEventCommand(void) const;
	void SetEventCommand(const shared_ptr<EventCommand>& command);

	bool GetEnableEventHandler(void) const;
	void SetEnableEventHandler(bool enabled);

	/* Flapping Detection */
	double GetFlappingCurrent(void) const;

	bool GetEnableFlapping(void) const;
	void SetEnableFlapping(bool enabled, const String& authority = String());

	bool IsFlapping(void) const;
	void UpdateFlappingStatus(bool stateChange);

	/* Performance data */
	bool GetEnablePerfdata(void) const;
	void SetEnablePerfdata(bool enabled, const String& authority = String());

protected:
	virtual void Start(void);

	virtual void OnConfigLoaded(void);
	virtual void OnStateLoaded(void);

private:
	Host::Ptr m_Host;

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
};

}

#endif /* SERVICE_H */
