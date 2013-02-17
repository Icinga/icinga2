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
 * The state type of a service.
 *
 * @ingroup icinga
 */
enum ServiceStateType
{
	StateTypeSoft,
	StateTypeHard
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

	Service(const Dictionary::Ptr& properties);
	~Service(void);

	static bool Exists(const String& name);
	static Service::Ptr GetByName(const String& name);

	static Service::Ptr GetByNamePair(const String& hostName, const String& serviceName);

	static const int DefaultMaxCheckAttempts;
	static const int DefaultCheckInterval;
	static const int CheckIntervalDivisor;

	String GetDisplayName(void) const;
	Host::Ptr GetHost(void) const;
	Dictionary::Ptr GetMacros(void) const;
	Dictionary::Ptr GetHostDependencies(void) const;
	Dictionary::Ptr GetServiceDependencies(void) const;
	Dictionary::Ptr GetGroups(void) const;
	String GetShortName(void) const;

	Dictionary::Ptr CalculateDynamicMacros(void) const;

	set<Host::Ptr> GetParentHosts(void) const;
	set<Service::Ptr> GetParentServices(void) const;

	bool IsReachable(void) const;

	AcknowledgementType GetAcknowledgement(void);
	void SetAcknowledgement(AcknowledgementType acknowledgement);

	/* Checks */
	Dictionary::Ptr GetCheckers(void) const;
	Value GetCheckCommand(void) const;
	long GetMaxCheckAttempts(void) const;
	double GetCheckInterval(void) const;
	double GetRetryInterval(void) const;

	long GetSchedulingOffset(void);
	void SetSchedulingOffset(long offset);

	void SetFirstCheck(bool first);
	bool GetFirstCheck(void) const;

	void SetNextCheck(double nextCheck);
	double GetNextCheck(void);
	void UpdateNextCheck(void);

	void SetChecker(const String& checker);
	String GetChecker(void) const;

	bool IsAllowedChecker(const String& checker) const;

	void SetCurrentCheckAttempt(long attempt);
	long GetCurrentCheckAttempt(void) const;

	void SetState(ServiceState state);
	ServiceState GetState(void) const;

	void SetStateType(ServiceStateType type);
	ServiceStateType GetStateType(void) const;

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

	void ApplyCheckResult(const Dictionary::Ptr& cr);
	static void UpdateStatistics(const Dictionary::Ptr& cr);

	void BeginExecuteCheck(const function<void (void)>& callback);
	void ProcessCheckResult(const Dictionary::Ptr& cr);

	static ServiceState StateFromString(const String& state);
	static String StateToString(ServiceState state);

	static ServiceStateType StateTypeFromString(const String& state);
	static String StateTypeToString(ServiceStateType state);

	static signals2::signal<void (const Service::Ptr&, const String&)> OnCheckerChanged;
	static signals2::signal<void (const Service::Ptr&, const Value&)> OnNextCheckChanged;

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
	static void ValidateDowntimesCache(void);

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
	static void ValidateCommentsCache(void);

	/* Notifications */
	void RequestNotifications(NotificationType type) const;
	void SendNotifications(NotificationType type);

	static void InvalidateNotificationsCache(void);
	static void ValidateNotificationsCache(void);

	set<Notification::Ptr> GetNotifications(void) const;

	void UpdateSlaveNotifications(void);

	double GetLastNotification(void) const;
	void SetLastNotification(double time);

	double GetNextNotification(void) const;
	void SetNextNotification(double time);

protected:
	virtual void OnAttributeChanged(const String& name, const Value& oldValue);

private:
	void CheckCompletedHandler(const Dictionary::Ptr& scheduleInfo,
	    const ScriptTask::Ptr& task, const function<void (void)>& callback);

	/* Downtimes */
	static int m_NextDowntimeID;

	static map<int, String> m_LegacyDowntimesCache;
	static map<String, Service::WeakPtr> m_DowntimesCache;
	static bool m_DowntimesCacheValid;
	static Timer::Ptr m_DowntimesExpireTimer;

	static void DowntimesExpireTimerHandler(void);

	void AddDowntimesToCache(void);
	void RemoveExpiredDowntimes(void);

	/* Comments */
	static int m_NextCommentID;

	static map<int, String> m_LegacyCommentsCache;
	static map<String, Service::WeakPtr> m_CommentsCache;
	static bool m_CommentsCacheValid;
	static Timer::Ptr m_CommentsExpireTimer;

	static void CommentsExpireTimerHandler(void);

	void AddCommentsToCache(void);
	void RemoveExpiredComments(void);

	/* Notifications */
	static map<String, set<Notification::WeakPtr> > m_NotificationsCache;
	static bool m_NotificationsCacheValid;
};

}

#endif /* SERVICE_H */
