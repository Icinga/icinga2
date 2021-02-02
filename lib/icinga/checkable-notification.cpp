/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/checkable.hpp"
#include "icinga/host.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/service.hpp"
#include "base/dictionary.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/context.hpp"
#include "base/convert.hpp"
#include "base/lazy-init.hpp"
#include "remote/apilistener.hpp"

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

	String notificationTypeName = Notification::NotificationTypeToString(type);

	// Bail early if there are no notifications.
	if (notifications.empty()) {
		Log(LogNotice, "Checkable")
			<< "Skipping checkable '" << checkableName << "' which doesn't have any notification objects configured.";
		return;
	}

	Log(LogInformation, "Checkable")
		<< "Checkable '" << checkableName << "' has " << notifications.size()
		<< " notification(s). Checking filters for type '" << notificationTypeName << "', sends will be logged.";

	for (const Notification::Ptr& notification : notifications) {
		// Re-send stashed notifications from cold startup.
		if (ApiListener::UpdatedObjectAuthority()) {
			try {
				if (!notification->IsPaused()) {
					auto stashedNotifications (notification->GetStashedNotifications());

					if (stashedNotifications->GetLength()) {
						Log(LogNotice, "Notification")
							<< "Notification '" << notification->GetName() << "': there are some stashed notifications. Stashing notification to preserve order.";

						stashedNotifications->Add(new Dictionary({
							{"type", type},
							{"cr", cr},
							{"force", force},
							{"reminder", false},
							{"author", author},
							{"text", text}
						}));
					} else {
						notification->BeginExecuteNotification(type, cr, force, false, author, text);
					}
				} else {
					Log(LogNotice, "Notification")
						<< "Notification '" << notification->GetName() << "': HA cluster active, this endpoint does not have the authority (paused=true). Skipping.";
				}
			} catch (const std::exception& ex) {
				Log(LogWarning, "Checkable")
					<< "Exception occurred during notification '" << notification->GetName() << "' for checkable '"
					<< GetName() << "': " << DiagnosticInformation(ex, false);
			}
		} else {
			// Cold startup phase. Stash notification for later.
			Log(LogNotice, "Notification")
				<< "Notification '" << notification->GetName() << "': object authority hasn't been updated, yet. Stashing notification.";

			notification->GetStashedNotifications()->Add(new Dictionary({
				{"type", type},
				{"cr", cr},
				{"force", force},
				{"reminder", false},
				{"author", author},
				{"text", text}
			}));
		}
	}
}

std::set<Notification::Ptr> Checkable::GetNotifications() const
{
	std::unique_lock<std::mutex> lock(m_NotificationMutex);
	return m_Notifications;
}

void Checkable::RegisterNotification(const Notification::Ptr& notification)
{
	std::unique_lock<std::mutex> lock(m_NotificationMutex);
	m_Notifications.insert(notification);
}

void Checkable::UnregisterNotification(const Notification::Ptr& notification)
{
	std::unique_lock<std::mutex> lock(m_NotificationMutex);
	m_Notifications.erase(notification);
}

static void FireSuppressedNotifications(Checkable* checkable)
{
	if (!checkable->IsActive())
		return;

	if (checkable->IsPaused())
		return;

	if (!checkable->GetEnableNotifications())
		return;

	int suppressed_types (checkable->GetSuppressedNotifications());
	if (!suppressed_types)
		return;

	int subtract = 0;

	{
		LazyInit<bool> wasLastParentRecoveryRecent ([&checkable]() {
			auto cr (checkable->GetLastCheckResult());

			if (!cr) {
				return true;
			}

			auto threshold (cr->GetExecutionStart());

			for (auto& dep : checkable->GetDependencies()) {
				auto parent (dep->GetParent());
				ObjectLock oLock (parent);

				if (!parent->GetProblem() && parent->GetLastStateChange() >= threshold) {
					return true;
				}
			}

			return false;
		});

		for (auto type : {NotificationProblem, NotificationRecovery, NotificationFlappingStart, NotificationFlappingEnd}) {
			if (suppressed_types & type) {
				bool still_applies = checkable->NotificationReasonApplies(type);

				if (still_applies) {
					bool still_suppressed;

					switch (type) {
						case NotificationProblem:
							/* Fall through. */
						case NotificationRecovery:
							still_suppressed = !checkable->IsReachable(DependencyNotification) || checkable->IsInDowntime() || checkable->IsAcknowledged();
							break;
						case NotificationFlappingStart:
							/* Fall through. */
						case NotificationFlappingEnd:
							still_suppressed = checkable->IsInDowntime();
							break;
						default:
							break;
					}

					if (!still_suppressed && !checkable->IsLikelyToBeCheckedSoon() && !wasLastParentRecoveryRecent.Get()) {
						Checkable::OnNotificationsRequested(checkable, type, checkable->GetLastCheckResult(), "", "", nullptr);

						subtract |= type;
					}
				} else {
					subtract |= type;
				}
			}
		}
	}

	if (subtract) {
		ObjectLock olock (checkable);

		int suppressed_types_before (checkable->GetSuppressedNotifications());
		int suppressed_types_after (suppressed_types_before & ~subtract);

		if (suppressed_types_after != suppressed_types_before) {
			checkable->SetSuppressedNotifications(suppressed_types_after);
		}
	}
}

/**
 * Re-sends all notifications previously suppressed by e.g. downtimes if the notification reason still applies.
 */
void Checkable::FireSuppressedNotifications(const Timer * const&)
{
	for (auto& host : ConfigType::GetObjectsByType<Host>()) {
		::FireSuppressedNotifications(host.get());
	}

	for (auto& service : ConfigType::GetObjectsByType<Service>()) {
		::FireSuppressedNotifications(service.get());
	}
}

/**
 * Returns whether sending a notification of type type right now would represent *this' current state correctly.
 *
 * @param type The type of notification to send (or not to send).
 *
 * @return Whether to send the notification.
 */
bool Checkable::NotificationReasonApplies(NotificationType type)
{
	switch (type) {
		case NotificationProblem:
			{
				auto cr (GetLastCheckResult());
				return cr && !IsStateOK(cr->GetState()) && GetStateType() == StateTypeHard;
			}
		case NotificationRecovery:
			{
				auto cr (GetLastCheckResult());
				return cr && IsStateOK(cr->GetState());
			}
		case NotificationFlappingStart:
			return IsFlapping();
		case NotificationFlappingEnd:
			return !IsFlapping();
		default:
			VERIFY(!"Checkable#NotificationReasonStillApplies(): given type not implemented");
			return false;
	}
}

/**
 * E.g. we're going to re-send a stashed problem notification as *this is still not ok.
 * But if the next check result recovers *this soon, we would send a recovery notification soon after the problem one.
 * This is not desired, especially for lots of checkables at once.
 * Because of that if there's likely to be a check result soon,
 * we delay the re-sending of the stashed notification until the next check.
 * That check either doesn't change anything and we finally re-send the stashed problem notification
 * or recovers *this and we drop the stashed notification.
 *
 * @return Whether *this is likely to be checked soon
 */
bool Checkable::IsLikelyToBeCheckedSoon()
{
	if (!GetEnableActiveChecks()) {
		return false;
	}

	// One minute unless the check interval is too short so the next check will always run during the next minute.
	auto threshold (GetCheckInterval() - 10);

	if (threshold > 60) {
		threshold = 60;
	} else if (threshold < 0) {
		threshold = 0;
	}

	return GetNextCheck() <= Utility::GetTime() + threshold;
}
