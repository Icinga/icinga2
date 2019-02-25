/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/checkable.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/context.hpp"
#include "base/convert.hpp"

using namespace icinga;

boost::signals2::signal<void (const Notification::Ptr&, const Checkable::Ptr&, const std::set<User::Ptr>&,
	const NotificationType&, const CheckResult::Ptr&, const String&, const String&,
	const MessageOrigin::Ptr&)> Checkable::OnNotificationSentToAllUsers;
boost::signals2::signal<void (const Notification::Ptr&, const Checkable::Ptr&, const User::Ptr&,
	const NotificationType&, const CheckResult::Ptr&, const String&, const String&, const String&,
	const MessageOrigin::Ptr&)> Checkable::OnNotificationSentToUser;

void Checkable::ResetNotificationNumbers()
{
	for (const Notification::Ptr& notification : GetNotifications()) {
		ObjectLock olock(notification);
		notification->ResetNotificationNumber();
	}
}

void Checkable::SendNotifications(NotificationType type, const CheckResult::Ptr& cr, const String& author, const String& text)
{
	String checkableName = GetName();

	CONTEXT("Sending notifications for object '" + checkableName + "'");

	bool force = GetForceNextNotification();

	SetForceNextNotification(false);

	if (!IcingaApplication::GetInstance()->GetEnableNotifications() || !GetEnableNotifications()) {
		if (!force) {
			Log(LogInformation, "Checkable")
				<< "Notifications are disabled for checkable '" << checkableName << "'.";
			return;
		}
	}

	std::set<Notification::Ptr> notifications = GetNotifications();

	Log(LogInformation, "Checkable")
		<< "Checkable '" << checkableName << "' has " << notifications.size() << " notification(s). Proceeding with filters, successful sends will be logged.";

	if (notifications.empty())
		return;

	for (const Notification::Ptr& notification : notifications) {
		try {
			if (!notification->IsPaused()) {
				notification->BeginExecuteNotification(type, cr, force, false, author, text);
			} else {
				Log(LogNotice, "Notification")
					<< "Notification '" << notification->GetName() << "': HA cluster active, this endpoint does not have the authority (paused=true). Skipping.";
			}
		} catch (const std::exception& ex) {
			Log(LogWarning, "Checkable")
				<< "Exception occurred during notification '" << notification->GetName() << "' for checkable '"
				<< GetName() << "': " << DiagnosticInformation(ex, false);
		}
	}
}

std::set<Notification::Ptr> Checkable::GetNotifications() const
{
	boost::mutex::scoped_lock lock(m_NotificationMutex);
	return m_Notifications;
}

void Checkable::RegisterNotification(const Notification::Ptr& notification)
{
	boost::mutex::scoped_lock lock(m_NotificationMutex);
	m_Notifications.insert(notification);
}

void Checkable::UnregisterNotification(const Notification::Ptr& notification)
{
	boost::mutex::scoped_lock lock(m_NotificationMutex);
	m_Notifications.erase(notification);
}
