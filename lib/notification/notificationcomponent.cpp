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

	Checkable::OnNotificationsRequested.connect(std::bind(&NotificationComponent::SendNotificationsHandler, this, _1,
		_2, _3, _4, _5));

	m_NotificationTimer = new Timer();
	m_NotificationTimer->SetInterval(5);
	m_NotificationTimer->OnTimerExpired.connect(std::bind(&NotificationComponent::NotificationTimerHandler, this));
	m_NotificationTimer->Start();
}

void NotificationComponent::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "NotificationComponent")
		<< "'" << GetName() << "' stopped.";

	ObjectImpl<NotificationComponent>::Stop(runtimeRemoved);
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

		/* Skip notification if paused, in a cluster setup & HA feature is enabled. */
		if (notification->IsPaused() && myEndpoint && GetEnableHA()) {
			Log(LogNotice, "NotificationComponent")
				<< "Reminder notification '" << notificationName << "': HA cluster active, this endpoint does not have the authority (paused=true). Skipping.";
			continue;
		}

		Checkable::Ptr checkable = notification->GetCheckable();

		if (!IcingaApplication::GetInstance()->GetEnableNotifications() || !checkable->GetEnableNotifications())
			continue;

		if (notification->GetInterval() <= 0 && notification->GetNoMoreNotifications()) {
			Log(LogNotice, "NotificationComponent")
				<< "Reminder notification '" << notificationName << "': Notification was sent out once and interval=0 disables reminder notifications.";
			continue;
		}

		if (notification->GetNextNotification() > now)
			continue;

		bool reachable = checkable->IsReachable(DependencyNotification);

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
