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

#include "notification/notificationcomponent.h"
#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(NotificationComponent);

NotificationComponent::NotificationComponent(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{ }

/**
 * Starts the component.
 */
void NotificationComponent::Start(void)
{
	m_Endpoint = Endpoint::MakeEndpoint("notification", false);
	m_Endpoint->RegisterTopicHandler("icinga::SendNotifications",
	    boost::bind(&NotificationComponent::SendNotificationsRequestHandler, this, _2,
	    _3));

	m_NotificationTimer = boost::make_shared<Timer>();
	m_NotificationTimer->SetInterval(5);
	m_NotificationTimer->OnTimerExpired.connect(boost::bind(&NotificationComponent::NotificationTimerHandler, this));
	m_NotificationTimer->Start();
}

/**
 * Stops the component.
 */
void NotificationComponent::Stop(void)
{
	m_Endpoint->Unregister();
}

/**
 * Periodically sends a notification::HelloWorld message.
 *
 * @param - Event arguments for the timer.
 */
void NotificationComponent::NotificationTimerHandler(void)
{
	double now = Utility::GetTime();

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Service")) {
		Service::Ptr service = dynamic_pointer_cast<Service>(object);
		bool reachable = service->IsReachable();

		bool send_notification;

		{
			ObjectLock olock(service);

			if (service->GetStateType() == StateTypeSoft)
				continue;

			if (service->GetState() == StateOK)
				continue;

			if (service->GetNotificationInterval() <= 0)
				continue;

			if (service->GetLastNotification() > now - service->GetNotificationInterval())
				continue;

			send_notification = reachable && !service->IsInDowntime() && !service->IsAcknowledged();
		}

		if (send_notification)
			service->RequestNotifications(NotificationProblem, service->GetLastCheckResult());
	}
}

/**
 * Processes icinga::SendNotifications messages.
 */
void NotificationComponent::SendNotificationsRequestHandler(const Endpoint::Ptr& sender,
    const RequestMessage& request)
{
	MessagePart params;
	if (!request.GetParams(&params))
		return;

	String svc;
	if (!params.Get("service", &svc))
		return;

	int type;
	if (!params.Get("type", &type))
		return;

	Dictionary::Ptr cr;
	if (!params.Get("check_result", &cr))
		return;

	Service::Ptr service = Service::GetByName(svc);

	if (!service)
		return;

	service->SendNotifications(static_cast<NotificationType>(type), cr);
}
