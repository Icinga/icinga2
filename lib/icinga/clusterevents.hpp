/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef CLUSTEREVENTS_H
#define CLUSTEREVENTS_H

#include "icinga/checkable.hpp"
#include "icinga/host.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"

namespace icinga
{

/**
 * @ingroup icinga
 */
class ClusterEvents
{
public:
	static void StaticInitialize();

	static void CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const MessageOrigin::Ptr& origin);
	static Value CheckResultAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void NextCheckChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);
	static Value NextCheckChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void LastCheckStartedChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);
	static Value LastCheckStartedChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void SuppressedNotificationsChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);
	static Value SuppressedNotificationsChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void SuppressedNotificationTypesChangedHandler(const Notification::Ptr& notification, const MessageOrigin::Ptr& origin);
	static Value SuppressedNotificationTypesChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void NextNotificationChangedHandler(const Notification::Ptr& notification, const MessageOrigin::Ptr& origin);
	static Value NextNotificationChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void ForceNextCheckChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);
	static Value ForceNextCheckChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void ForceNextNotificationChangedHandler(const Checkable::Ptr& checkable, const MessageOrigin::Ptr& origin);
	static Value ForceNextNotificationChangedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void AcknowledgementSetHandler(const Checkable::Ptr& checkable, const String& author, const String& comment, AcknowledgementType type,
		bool notify, bool persistent, double changeTime, double expiry, const MessageOrigin::Ptr& origin);
	static Value AcknowledgementSetAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void AcknowledgementClearedHandler(const Checkable::Ptr& checkable, const String& removedBy, double changeTime, const MessageOrigin::Ptr& origin);
	static Value AcknowledgementClearedAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static Value ExecuteCommandAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static Dictionary::Ptr MakeCheckResultMessage(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr);

	static void SendNotificationsHandler(const Checkable::Ptr& checkable, NotificationType type,
		const CheckResult::Ptr& cr, const String& author, const String& text, const MessageOrigin::Ptr& origin);
	static Value SendNotificationsAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void NotificationSentUserHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable, const User::Ptr& user,
		NotificationType notificationType, const CheckResult::Ptr& cr, const String& author, const String& commentText, const String& command, const MessageOrigin::Ptr& origin);
	static Value NotificationSentUserAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static void NotificationSentToAllUsersHandler(const Notification::Ptr& notification, const Checkable::Ptr& checkable, const std::set<User::Ptr>& users,
		NotificationType notificationType, const CheckResult::Ptr& cr, const String& author, const String& commentText, const MessageOrigin::Ptr& origin);
	static Value NotificationSentToAllUsersAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static Value ExecutedCommandAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static Value UpdateExecutionsAPIHandler(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);

	static int GetCheckRequestQueueSize();
	static void LogRemoteCheckQueueInformation();

private:
	static std::mutex m_Mutex;
	static std::deque<std::function<void ()>> m_CheckRequestQueue;
	static bool m_CheckSchedulerRunning;
	static int m_ChecksExecutedDuringInterval;
	static int m_ChecksDroppedDuringInterval;
	static Timer::Ptr m_LogTimer;

	static void RemoteCheckThreadProc();
	static void EnqueueCheck(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
	static void ExecuteCheckFromQueue(const MessageOrigin::Ptr& origin, const Dictionary::Ptr& params);
};

}

#endif /* CLUSTEREVENTS_H */
