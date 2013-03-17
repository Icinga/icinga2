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
#include "icinga/host.h"
#include "icinga/timeperiod.h"
#include "icinga/notification.h"
#include "base/dynamicobject.h"
#include "base/array.h"
#include <boost/signals2.hpp>

namespace icinga
{

/**
 * The state of a service.
 *
 * @ingroup icinga
 */
enum ServiceState
{
	StateOK,
	StateWarning,
	StateCritical,
	StateUnknown,
	StateUncheckable,
};

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

class CheckResultMessage;

/**
 * An Icinga service.
 *
 * @ingroup icinga
 */
class I2_ICINGA_API Service : public DynamicObject
{
public:
	typedef shared_ptr<Service> Ptr;
	typedef weak_ptr<Service> WeakPtr;

	explicit Service(const Dictionary::Ptr& serializedUpdate);
	~Service(void);

	static Service::Ptr GetByName(const String& name);

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

	Dictionary::Ptr CalculateDynamicMacros(const Dictionary::Ptr& crOverride = Dictionary::Ptr()) const;
	Dictionary::Ptr CalculateAllMacros(const Dictionary::Ptr& crOverride = Dictionary::Ptr()) const;

	std::set<Host::Ptr> GetParentHosts(void) const;
	std::set<Service::Ptr> GetParentServices(void) const;

	bool IsReachable(void) const;

	AcknowledgementType GetAcknowledgement(void);
	void SetAcknowledgement(AcknowledgementType acknowledgement);

	/* Checks */
	Array::Ptr GetCheckers(void) const;
	Value GetCheckCommand(void) const;
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

	void SetLastStateType(StateType type);
	StateType GetLastStateType(void) const;

	void SetLastCheckResult(const Dictionary::Ptr& result);
	Dictionary::Ptr GetLastCheckResult(void) const;

	void SetLastStateChange(double ts);
	double GetLastStateChange(void) const;

	void SetLastHardStateChange(double ts);
	double GetLastHardStateChange(void) const;

	bool GetEnableActiveChecks(void) const;
	void SetEnableActiveChecks(bool enabled);

	bool GetEnablePassiveChecks(void) const;
	void SetEnablePassiveChecks(bool enabled);

	bool GetForceNextCheck(void) const;
	void SetForceNextCheck(bool forced);

	double GetAcknowledgementExpiry(void) const;
	void SetAcknowledgementExpiry(double timestamp);

	static void UpdateStatistics(const Dictionary::Ptr& cr);

	void AcknowledgeProblem(AcknowledgementType type, double expiry = 0);
	void ClearAcknowledgement(void);

	void BeginExecuteCheck(const boost::function<void (void)>& callback);
	void ProcessCheckResult(const Dictionary::Ptr& cr);

	static double CalculateExecutionTime(const Dictionary::Ptr& cr);
	static double CalculateLatency(const Dictionary::Ptr& cr);

	static ServiceState StateFromString(const String& state);
	static String StateToString(ServiceState state);

	static StateType StateTypeFromString(const String& state);
	static String StateTypeToString(StateType state);

	static boost::signals2::signal<void (const Service::Ptr&)> OnCheckerChanged;
	static boost::signals2::signal<void (const Service::Ptr&)> OnNextCheckChanged;

	/* Downtimes */
	static int GetNextDowntimeID(void);

	Dictionary::Ptr GetDowntimes(void) const;

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

	static void InvalidateDowntimesCache(void);

	bool IsInDowntime(void) const;
	bool IsAcknowledged(void);

	/* Comments */
	static int GetNextCommentID(void);

	Dictionary::Ptr GetComments(void) const;

	String AddComment(CommentType entryType, const String& author,
	    const String& text, double expireTime);

	void RemoveAllComments(void);
	static void RemoveComment(const String& id);

	static String GetCommentIDFromLegacyID(int id);
	static Service::Ptr GetOwnerByCommentID(const String& id);
	static Dictionary::Ptr GetCommentByID(const String& id);

	static bool IsCommentExpired(const Dictionary::Ptr& comment);

	static void InvalidateCommentsCache(void);

	/* Notifications */
	bool GetEnableNotifications(void) const;
	void SetEnableNotifications(bool enabled);

	double GetNotificationInterval(void) const;

	void RequestNotifications(NotificationType type, const Dictionary::Ptr& cr);
	void SendNotifications(NotificationType type, const Dictionary::Ptr& cr);

	std::set<Notification::Ptr> GetNotifications(void) const;

	static void InvalidateNotificationsCache(void);

	void UpdateSlaveNotifications(void);

	double GetLastNotification(void) const;
	void SetLastNotification(double time);

protected:
	virtual void OnRegistrationCompleted(void);
	virtual void OnAttributeChanged(const String& name);

private:
	Dictionary::Ptr m_SlaveNotifications;

	Attribute<String> m_DisplayName;
	Attribute<Dictionary::Ptr> m_Macros;
	Attribute<Array::Ptr> m_HostDependencies;
	Attribute<Array::Ptr> m_ServiceDependencies;
	Attribute<Array::Ptr> m_ServiceGroups;
	Attribute<String> m_ShortName;
	Attribute<long> m_Acknowledgement;
	Attribute<double> m_AcknowledgementExpiry;
	Attribute<String> m_HostName;

	/* Checks */
	Attribute<Value> m_CheckCommand;
	Attribute<long> m_MaxCheckAttempts;
	Attribute<String> m_CheckPeriod;
	Attribute<double> m_CheckInterval;
	Attribute<double> m_RetryInterval;
	Attribute<double> m_NextCheck;
	Attribute<Array::Ptr> m_Checkers;
	Attribute<String> m_CurrentChecker;
	Attribute<long> m_CheckAttempt;
	Attribute<long> m_State;
	Attribute<long> m_StateType;
	Attribute<long> m_LastState;
	Attribute<long> m_LastStateType;
	Attribute<Dictionary::Ptr> m_LastResult;
	Attribute<double> m_LastStateChange;
	Attribute<double> m_LastHardStateChange;
	Attribute<bool> m_EnableActiveChecks;
	Attribute<bool> m_EnablePassiveChecks;
	Attribute<bool> m_ForceNextCheck;

	ScriptTask::Ptr m_CurrentTask;
	bool m_CheckRunning;
	long m_SchedulingOffset;

	void CheckCompletedHandler(const Dictionary::Ptr& checkInfo,
	    const ScriptTask::Ptr& task, const boost::function<void (void)>& callback);

	/* Downtimes */
	Attribute<Dictionary::Ptr> m_Downtimes;

	static int m_NextDowntimeID;

	static boost::mutex m_DowntimeMutex;
	static std::map<int, String> m_LegacyDowntimesCache;
	static std::map<String, Service::WeakPtr> m_DowntimesCache;
	static bool m_DowntimesCacheNeedsUpdate;
	static Timer::Ptr m_DowntimesCacheTimer;
	static Timer::Ptr m_DowntimesExpireTimer;

	static void DowntimesExpireTimerHandler(void);

	void RemoveExpiredDowntimes(void);

	static void RefreshDowntimesCache(void);

	/* Comments */
	Attribute<Dictionary::Ptr> m_Comments;

	static int m_NextCommentID;

	static boost::mutex m_CommentMutex;
	static std::map<int, String> m_LegacyCommentsCache;
	static std::map<String, Service::WeakPtr> m_CommentsCache;
	static bool m_CommentsCacheNeedsUpdate;
	static Timer::Ptr m_CommentsCacheTimer;
	static Timer::Ptr m_CommentsExpireTimer;

	static void CommentsExpireTimerHandler(void);

	void AddCommentsToCache(void);
	void RemoveExpiredComments(void);

	static void RefreshCommentsCache(void);

	/* Notifications */
	Attribute<bool> m_EnableNotifications;
	Attribute<double> m_LastNotification;
	Attribute<double> m_NotificationInterval;

	static boost::mutex m_NotificationMutex;
	static std::map<String, std::set<Notification::WeakPtr> > m_NotificationsCache;
	static bool m_NotificationsCacheNeedsUpdate;
	static Timer::Ptr m_NotificationsCacheTimer;

	static void RefreshNotificationsCache(void);
};

}

#endif /* SERVICE_H */
