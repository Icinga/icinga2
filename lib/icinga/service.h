/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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
#include "icinga/macroresolver.h"
#include "icinga/host.h"
#include "icinga/timeperiod.h"
#include "icinga/notification.h"
#include "base/i2-base.h"
#include "base/dynamicobject.h"
#include "base/array.h"
#include <boost/signals2.hpp>
#include <boost/thread/once.hpp>

namespace icinga
{

/**
 * The acknowledgement type of a service.
 *
 * @ingroup icinga
 */
enum AcknowledgementType
{
	AcknowledgementNone = 0,
	AcknowledgementNormal = 1,
	AcknowledgementSticky = 2
};

/**
 * The type of a service comment.
 *
 * @ingroup icinga
 */
enum CommentType
{
	CommentUser = 1,
	CommentDowntime = 2,
	CommentFlapping = 3,
	CommentAcknowledgement = 4
};

/**
 * The state of a service downtime.
 *
 * @ingroup icinga
 */
enum DowntimeState
{
	DowntimeStarted = 0,
	DowntimeCancelled = 1,
	DowntimeStopped = 2
};

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
 * The state of a changed comment
 *
 * @ingroup icinga
 */
enum CommentChangedType
{
	CommentChangedAdded = 0,
	CommentChangedUpdated = 1,
	CommentChangedDeleted = 2
};

/**
 * The state of a changed downtime
 *
 * @ingroup icinga
 */
enum DowntimeChangedType
{
	DowntimeChangedAdded = 0,
	DowntimeChangedUpdated = 1,
	DowntimeChangedDeleted = 2
};

class CheckCommand;
class EventCommand;

/**
 * An Icinga service.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Service : public DynamicObject, public MacroResolver
{
public:
	DECLARE_PTR_TYPEDEFS(Service);
	DECLARE_TYPENAME(Service);

	static Service::Ptr GetByNamePair(const String& hostName, const String& serviceName);

	static const int DefaultMaxCheckAttempts;
	static const double DefaultCheckInterval;
	static const double CheckIntervalDivisor;

	String GetDisplayName(void) const;
	Host::Ptr GetHost(void) const;
	Dictionary::Ptr GetMacros(void) const;
	Array::Ptr GetHostDependencies(void) const;
	Array::Ptr GetServiceDependencies(void) const;
	Array::Ptr GetGroups(void) const;
	String GetHostName(void) const;
	String GetShortName(void) const;

	std::set<Host::Ptr> GetParentHosts(void) const;
	std::set<Service::Ptr> GetParentServices(void) const;

	bool IsHostCheck(void) const;

	bool IsVolatile(void) const;

	bool IsReachable(void) const;

	AcknowledgementType GetAcknowledgement(void);
	void SetAcknowledgement(AcknowledgementType acknowledgement);

	/* Checks */
	Array::Ptr GetCheckers(void) const;
	shared_ptr<CheckCommand> GetCheckCommand(void) const;
	long GetMaxCheckAttempts(void) const;
	TimePeriod::Ptr GetCheckPeriod(void) const;
	double GetCheckInterval(void) const;
	double GetRetryInterval(void) const;

	long GetSchedulingOffset(void);
	void SetSchedulingOffset(long offset);

	void SetNextCheck(double nextCheck);
	double GetNextCheck(void);
	void UpdateNextCheck(void);

	void SetCurrentChecker(const String& checker);
	String GetCurrentChecker(void) const;

	bool IsAllowedChecker(const String& checker) const;

	void SetCurrentCheckAttempt(long attempt);
	long GetCurrentCheckAttempt(void) const;

	void SetState(ServiceState state);
	ServiceState GetState(void) const;

	void SetStateType(StateType type);
	StateType GetStateType(void) const;

	void SetLastState(ServiceState state);
	ServiceState GetLastState(void) const;

	void SetLastHardState(ServiceState state);
	ServiceState GetLastHardState(void) const;

	void SetLastStateType(StateType type);
	StateType GetLastStateType(void) const;

	void SetLastCheckResult(const Dictionary::Ptr& result);
	Dictionary::Ptr GetLastCheckResult(void) const;
	bool HasBeenChecked(void) const;

	double GetLastCheck(void) const;

	String GetLastCheckOutput(void) const;
	String GetLastCheckLongOutput(void) const;
	String GetLastCheckPerfData(void) const;

	void SetLastStateChange(double ts);
	double GetLastStateChange(void) const;

	void SetLastHardStateChange(double ts);
	double GetLastHardStateChange(void) const;

	void SetLastStateOK(double ts);
	double GetLastStateOK(void) const;
	void SetLastStateWarning(double ts);
	double GetLastStateWarning(void) const;
	void SetLastStateCritical(double ts);
	double GetLastStateCritical(void) const;
	void SetLastStateUnknown(double ts);
	double GetLastStateUnknown(void) const;
	void SetLastStateUnreachable(double ts);
	double GetLastStateUnreachable(void) const;

	void SetLastReachable(bool reachable);
	bool GetLastReachable(void) const;

	bool GetEnableActiveChecks(void) const;
	void SetEnableActiveChecks(bool enabled);

	bool GetEnablePassiveChecks(void) const;
	void SetEnablePassiveChecks(bool enabled);

	bool GetForceNextCheck(void) const;
	void SetForceNextCheck(bool forced);

	double GetAcknowledgementExpiry(void) const;
	void SetAcknowledgementExpiry(double timestamp);

	static void UpdateStatistics(const Dictionary::Ptr& cr);

	void AcknowledgeProblem(const String& author, const String& comment, AcknowledgementType type, double expiry = 0);
	void ClearAcknowledgement(void);

	void ExecuteCheck(void);
	void ProcessCheckResult(const Dictionary::Ptr& cr);

	static double CalculateExecutionTime(const Dictionary::Ptr& cr);
	static double CalculateLatency(const Dictionary::Ptr& cr);

	static ServiceState StateFromString(const String& state);
	static String StateToString(ServiceState state);

	static StateType StateTypeFromString(const String& state);
	static String StateTypeToString(StateType state);

	static boost::signals2::signal<void (const Service::Ptr&)> OnNextCheckChanged;
	static boost::signals2::signal<void (const Service::Ptr&, const Dictionary::Ptr&)> OnNewCheckResult;
	static boost::signals2::signal<void (const Service::Ptr&, NotificationType, const Dictionary::Ptr&, const String&, const String&)> OnNotificationsRequested;
	static boost::signals2::signal<void (const Service::Ptr&, const User::Ptr&, const NotificationType&, const Dictionary::Ptr&, const String&, const String&)> OnNotificationSentChanged;
	static boost::signals2::signal<void (const Service::Ptr&, DowntimeState)> OnDowntimeChanged;
	static boost::signals2::signal<void (const Service::Ptr&, FlappingState)> OnFlappingChanged;
	static boost::signals2::signal<void (const Service::Ptr&, const String&, CommentChangedType)> OnCommentsChanged;
	static boost::signals2::signal<void (const Service::Ptr&, const String&, DowntimeChangedType)> OnDowntimesChanged;

	virtual bool ResolveMacro(const String& macro, const Dictionary::Ptr& cr, String *result) const;

	/* Downtimes */
	static int GetNextDowntimeID(void);

	Dictionary::Ptr GetDowntimes(void) const;
	int GetDowntimeDepth(void) const;

	String AddDowntime(const String& author, const String& comment,
	    double startTime, double endTime, bool fixed,
	    const String& triggeredBy, double duration);

	static void RemoveDowntime(const String& id);

        void TriggerDowntimes(void);
	static void TriggerDowntime(const String& id);

	static String GetDowntimeIDFromLegacyID(int id);
	static Service::Ptr GetOwnerByDowntimeID(const String& id);
	static Dictionary::Ptr GetDowntimeByID(const String& id);

	static bool IsDowntimeActive(const Dictionary::Ptr& downtime);
	static bool IsDowntimeExpired(const Dictionary::Ptr& downtime);

	bool IsInDowntime(void) const;
	bool IsAcknowledged(void);

	/* Comments */
	static int GetNextCommentID(void);

	Dictionary::Ptr GetComments(void) const;

	String AddComment(CommentType entryType, const String& author,
	    const String& text, double expireTime);

	void RemoveAllComments(void);
	void RemoveCommentsByType(int type);
	static void RemoveComment(const String& id);

	static String GetCommentIDFromLegacyID(int id);
	static Service::Ptr GetOwnerByCommentID(const String& id);
	static Dictionary::Ptr GetCommentByID(const String& id);

	static bool IsCommentExpired(const Dictionary::Ptr& comment);

	/* Notifications */
	Dictionary::Ptr GetNotificationDescriptions(void) const;

	bool GetEnableNotifications(void) const;
	void SetEnableNotifications(bool enabled);

	void SendNotifications(NotificationType type, const Dictionary::Ptr& cr, const String& author = "", const String& text = "");

	std::set<Notification::Ptr> GetNotifications(void) const;
	void AddNotification(const Notification::Ptr& notification);
	void RemoveNotification(const Notification::Ptr& notification);

	void SetForceNextNotification(bool force);
	bool GetForceNextNotification(void) const;

	void ResetNotificationNumbers(void);

	void UpdateSlaveNotifications(void);

	/* Event Handler */
	void ExecuteEventHandler(void);
	shared_ptr<EventCommand> GetEventCommand(void) const;

	/* Flapping Detection */
	bool GetEnableFlapping(void) const;
	void SetEnableFlapping(bool enabled);

	double GetFlappingCurrent(void) const;
	double GetFlappingThreshold(void) const;

	bool IsFlapping(void) const;
	void UpdateFlappingStatus(bool stateChange);

protected:
	virtual void Start(void);

	virtual void InternalSerialize(const Dictionary::Ptr& bag, int attributeTypes) const;
	virtual void InternalDeserialize(const Dictionary::Ptr& bag, int attributeTypes);

private:
	String m_DisplayName;
	Dictionary::Ptr m_Macros;
	Array::Ptr m_HostDependencies;
	Array::Ptr m_ServiceDependencies;
	Array::Ptr m_ServiceGroups;
	String m_ShortName;
	Value m_Acknowledgement;
	Value m_AcknowledgementExpiry;
	String m_HostName;
	Value m_Volatile;

	/* Checks */
	String m_CheckCommand;
	Value m_MaxCheckAttempts;
	String m_CheckPeriod;
	Value m_CheckInterval;
	Value m_RetryInterval;
	double m_NextCheck;
	Array::Ptr m_Checkers;
	String m_CurrentChecker;
	Value m_CheckAttempt;
	Value m_State;
	Value m_StateType;
	Value m_LastState;
	Value m_LastHardState;
	Value m_LastStateType;
	Value m_LastReachable;
	Dictionary::Ptr m_LastResult;
	Value m_LastStateChange;
	Value m_LastHardStateChange;
	Value m_LastStateOK;
	Value m_LastStateWarning;
	Value m_LastStateCritical;
	Value m_LastStateUnknown;
	Value m_LastStateUnreachable;
	bool m_LastInDowntime;
	Value m_EnableActiveChecks;
	Value m_EnablePassiveChecks;
	Value m_ForceNextCheck;

	bool m_CheckRunning;
	long m_SchedulingOffset;

	/* Downtimes */
	Dictionary::Ptr m_Downtimes;

	static void DowntimesExpireTimerHandler(void);

	void RemoveExpiredDowntimes(void);

	void AddDowntimesToCache(void);

	/* Comments */
	Dictionary::Ptr m_Comments;

	static void CommentsExpireTimerHandler(void);

	void RemoveExpiredComments(void);

	void AddCommentsToCache(void);

	/* Notifications */
	Dictionary::Ptr m_NotificationDescriptions;

	Value m_EnableNotifications;
	Value m_ForceNextNotification;

	std::set<Notification::Ptr> m_Notifications;

	/* Event Handler */
	String m_EventCommand;

	/* Flapping */
	Value m_EnableFlapping;
	long m_FlappingPositive;
	long m_FlappingNegative;
	Value m_FlappingLastChange;
	Value m_FlappingThreshold;
};

}

#endif /* SERVICE_H */
