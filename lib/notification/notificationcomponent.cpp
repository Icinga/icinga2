/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "notification/notificationcomponent.hpp"
#include "notification/notificationcomponent.tcpp"
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
	Dictionary::Ptr nodes = new Dictionary();

	for (const NotificationComponent::Ptr& notification_component : ConfigType::GetObjectsByType<NotificationComponent>()) {
		nodes->Set(notification_component->GetName(), 1); //add more stats
	}

	status->Set("notificationcomponent", nodes);
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

	for (const Notification::Ptr& notification : ConfigType::GetObjectsByType<Notification>()) {
		if (!notification->IsActive())
			continue;

		if (notification->IsPaused() && GetEnableHA())
			continue;

		Checkable::Ptr checkable = notification->GetCheckable();

		if (!IcingaApplication::GetInstance()->GetEnableNotifications() || !checkable->GetEnableNotifications())
			continue;

		if (notification->GetInterval() <= 0 && notification->GetNoMoreNotifications())
			continue;

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

			if ((service && service->GetState() == ServiceOK) || (!service && host->GetState() == HostUp))
				continue;

			if (!reachable || checkable->IsInDowntime() || checkable->IsAcknowledged())
				continue;
		}

		try {
			Log(LogNotice, "NotificationComponent")
				<< "Attempting to send reminder notification '" << notification->GetName() << "'";
			notification->BeginExecuteNotification(NotificationProblem, checkable->GetLastCheckResult(), false, true);
		} catch (const std::exception& ex) {
			Log(LogWarning, "NotificationComponent")
				<< "Exception occured during notification for object '"
				<< GetName() << "': " << DiagnosticInformation(ex);
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
