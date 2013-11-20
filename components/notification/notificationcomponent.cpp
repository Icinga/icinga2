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

#include "notification/notificationcomponent.h"
#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/utility.h"
#include "base/exception.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(NotificationComponent);

/**
 * Starts the component.
 */
void NotificationComponent::Start(void)
{
	DynamicObject::Start();

	Service::OnNotificationsRequested.connect(boost::bind(&NotificationComponent::SendNotificationsHandler, this, _1,
	    _2, _3, _4, _5));

	m_NotificationTimer = make_shared<Timer>();
	m_NotificationTimer->SetInterval(5);
	m_NotificationTimer->OnTimerExpired.connect(boost::bind(&NotificationComponent::NotificationTimerHandler, this));
	m_NotificationTimer->Start();
}

/**
 * Periodically sends notifications.
 *
 * @param - Event arguments for the timer.
 */
void NotificationComponent::NotificationTimerHandler(void)
{
	double now = Utility::GetTime();

	BOOST_FOREACH(const Notification::Ptr& notification, DynamicType::GetObjects<Notification>()) {
		Service::Ptr service = notification->GetService();

		if (notification->GetNotificationInterval() <= 0 && notification->GetLastProblemNotification() < service->GetLastHardStateChange())
			continue;

		if (notification->GetNextNotification() > now)
			continue;

		bool reachable = service->IsReachable();

		{
			ObjectLock olock(notification);
			notification->SetNextNotification(Utility::GetTime() + notification->GetNotificationInterval());
		}

		{
			ObjectLock olock(service);

			if (service->GetStateType() == StateTypeSoft)
				continue;

			if (service->GetState() == StateOK)
				continue;

			if (!reachable || service->IsInDowntime() || service->IsAcknowledged())
				continue;
		}

		try {
			Log(LogInformation, "notification", "Sending reminder notification for service '" + service->GetName() + "'");
			notification->BeginExecuteNotification(NotificationProblem, service->GetLastCheckResult(), false);
		} catch (const std::exception& ex) {
			std::ostringstream msgbuf;
			msgbuf << "Exception occured during notification for service '"
			       << GetName() << "': " << DiagnosticInformation(ex);
			String message = msgbuf.str();

			Log(LogWarning, "icinga", message);
		}
	}
}

/**
 * Processes icinga::SendNotifications messages.
 */
void NotificationComponent::SendNotificationsHandler(const Service::Ptr& service, NotificationType type,
    const CheckResult::Ptr& cr, const String& author, const String& text)
{
	service->SendNotifications(type, cr, author, text);
}
