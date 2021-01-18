/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "notification/notificationcomponent.hpp"
#include "notification/notificationcomponent-ti.cpp"
#include "icinga/service.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/exception.hpp"
#include "base/statsfunction.hpp"
#include "remote/apilistener.hpp"

using namespace icinga;

REGISTER_TYPE(NotificationComponent);

REGISTER_STATSFUNCTION(NotificationComponent, &NotificationComponent::StatsFunc);

void NotificationComponent::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	DictionaryData nodes;

	for (const NotificationComponent::Ptr& notification_component : ConfigType::GetObjectsByType<NotificationComponent>()) {
		nodes.emplace_back(notification_component->GetName(), 1); //add more stats
	}

	status->Set("notificationcomponent", new Dictionary(std::move(nodes)));
}

/**
 * Starts the component.
 */
void NotificationComponent::Start(bool runtimeCreated)
{
	ObjectImpl<NotificationComponent>::Start(runtimeCreated);

	Log(LogInformation, "NotificationComponent")
		<< "'" << GetName() << "' started.";

	Checkable::OnNotificationsRequested.connect([this](const Checkable::Ptr& checkable, NotificationType type, const CheckResult::Ptr& cr,
		const String& author, const String& text, const MessageOrigin::Ptr&) {
		SendNotificationsHandler(checkable, type, cr, author, text);
	});

	m_NotificationTimer = new Timer();
	m_NotificationTimer->SetInterval(5);
	m_NotificationTimer->OnTimerExpired.connect([this](const Timer * const&) { NotificationTimerHandler(); });
	m_NotificationTimer->Start();
}

void NotificationComponent::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "NotificationComponent")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<NotificationComponent>::Stop(runtimeRemoved);
}

static inline
void SubtractSuppressedNotificationTypes(const Notification::Ptr& notification, int types)
{
	ObjectLock olock (notification);

	int suppressedTypesBefore (notification->GetSuppressedNotifications());
	int suppressedTypesAfter (suppressedTypesBefore & ~types);

	if (suppressedTypesAfter != suppressedTypesBefore) {
		notification->SetSuppressedNotifications(suppressedTypesAfter);
	}
}

static inline
void FireSuppressedNotifications(const Notification::Ptr& notification)
{
	int suppressedTypes (notification->GetSuppressedNotifications());
	if (!suppressedTypes)
		return;

	int subtract = 0;
	auto checkable (notification->GetCheckable());

	for (auto type : {NotificationProblem, NotificationRecovery, NotificationFlappingStart, NotificationFlappingEnd}) {
		if ((suppressedTypes & type) && !checkable->NotificationReasonApplies(type)) {
			subtract |= type;
			suppressedTypes &= ~type;
		}
	}

	if (suppressedTypes) {
		auto tp (notification->GetPeriod());

		if ((!tp || tp->IsInside(Utility::GetTime())) && !checkable->IsLikelyToBeCheckedSoon()) {
			for (auto type : {NotificationProblem, NotificationRecovery, NotificationFlappingStart, NotificationFlappingEnd}) {
				if (!(suppressedTypes & type))
					continue;

				auto notificationName (notification->GetName());

				Log(LogNotice, "NotificationComponent")
					<< "Attempting to re-send previously suppressed notification '" << notificationName << "'.";

				subtract |= type;
				SubtractSuppressedNotificationTypes(notification, subtract);
				subtract = 0;

				try {
					notification->BeginExecuteNotification(type, checkable->GetLastCheckResult(), false, false);
				} catch (const std::exception& ex) {
					Log(LogWarning, "NotificationComponent")
						<< "Exception occurred during notification for object '"
						<< notificationName << "': " << DiagnosticInformation(ex, false);
				}
			}
		}
	}

	if (subtract) {
		SubtractSuppressedNotificationTypes(notification, subtract);
	}
}

/**
 * Periodically sends notifications.
 *
 * @param - Event arguments for the timer.
 */
void NotificationComponent::NotificationTimerHandler()
{
	double now = Utility::GetTime();

	/* Function already checks whether 'api' feature is enabled. */
	Endpoint::Ptr myEndpoint = Endpoint::GetLocalEndpoint();

	for (const Notification::Ptr& notification : ConfigType::GetObjectsByType<Notification>()) {
		if (!notification->IsActive())
			continue;

		String notificationName = notification->GetName();
		bool updatedObjectAuthority = ApiListener::UpdatedObjectAuthority();

		/* Skip notification if paused, in a cluster setup & HA feature is enabled. */
		if (notification->IsPaused()) {
			if (updatedObjectAuthority) {
				auto stashedNotifications (notification->GetStashedNotifications());
				ObjectLock olock(stashedNotifications);

				if (stashedNotifications->GetLength()) {
					Log(LogNotice, "NotificationComponent")
						<< "Notification '" << notificationName << "': HA cluster active, this endpoint does not have the authority. Dropping all stashed notifications.";

					stashedNotifications->Clear();
				}
			}

			if (myEndpoint && GetEnableHA()) {
				Log(LogNotice, "NotificationComponent")
					<< "Reminder notification '" << notificationName << "': HA cluster active, this endpoint does not have the authority (paused=true). Skipping.";
				continue;
			}
		}

		Checkable::Ptr checkable = notification->GetCheckable();

		if (!IcingaApplication::GetInstance()->GetEnableNotifications() || !checkable->GetEnableNotifications())
			continue;

		bool reachable = checkable->IsReachable(DependencyNotification);

		if (reachable) {
			{
				Array::Ptr unstashedNotifications = new Array();

				{
					auto stashedNotifications (notification->GetStashedNotifications());
					ObjectLock olock(stashedNotifications);

					stashedNotifications->CopyTo(unstashedNotifications);
					stashedNotifications->Clear();
				}

				ObjectLock olock(unstashedNotifications);

				for (Dictionary::Ptr unstashedNotification : unstashedNotifications) {
					try {
						Log(LogNotice, "NotificationComponent")
							<< "Attempting to send stashed notification '" << notificationName << "'.";

						notification->BeginExecuteNotification(
							(NotificationType)(int)unstashedNotification->Get("type"),
							(CheckResult::Ptr)unstashedNotification->Get("cr"),
							(bool)unstashedNotification->Get("force"),
							(bool)unstashedNotification->Get("reminder"),
							(String)unstashedNotification->Get("author"),
							(String)unstashedNotification->Get("text")
						);
					} catch (const std::exception& ex) {
						Log(LogWarning, "NotificationComponent")
							<< "Exception occurred during notification for object '"
							<< notificationName << "': " << DiagnosticInformation(ex, false);
					}
				}
			}

			FireSuppressedNotifications(notification);
		}

		if (notification->GetInterval() <= 0 && notification->GetNoMoreNotifications()) {
			Log(LogNotice, "NotificationComponent")
				<< "Reminder notification '" << notificationName << "': Notification was sent out once and interval=0 disables reminder notifications.";
			continue;
		}

		if (notification->GetNextNotification() > now)
			continue;

		{
			ObjectLock olock(notification);
			notification->SetNextNotification(Utility::GetTime() + notification->GetInterval());
		}

		{
			Host::Ptr host;
			Service::Ptr service;
			tie(host, service) = GetHostService(checkable);

			ObjectLock olock(checkable);

			if (checkable->GetStateType() == StateTypeSoft)
				continue;

			/* Don't send reminder notifications for OK/Up states. */
			if ((service && service->GetState() == ServiceOK) || (!service && host->GetState() == HostUp))
				continue;

			/* Don't send reminder notifications before initial ones. */
			if (checkable->GetSuppressedNotifications() & NotificationProblem)
				continue;

			/* Skip in runtime filters. */
			if (!reachable || checkable->IsInDowntime() || checkable->IsAcknowledged() || checkable->IsFlapping())
				continue;
		}

		try {
			Log(LogNotice, "NotificationComponent")
				<< "Attempting to send reminder notification '" << notificationName << "'.";

			notification->BeginExecuteNotification(NotificationProblem, checkable->GetLastCheckResult(), false, true);
		} catch (const std::exception& ex) {
			Log(LogWarning, "NotificationComponent")
				<< "Exception occurred during notification for object '"
				<< notificationName << "': " << DiagnosticInformation(ex, false);
		}
	}
}

/**
 * Processes icinga::SendNotifications messages.
 */
void NotificationComponent::SendNotificationsHandler(const Checkable::Ptr& checkable, NotificationType type,
	const CheckResult::Ptr& cr, const String& author, const String& text)
{
	checkable->SendNotifications(type, cr, author, text);
}
