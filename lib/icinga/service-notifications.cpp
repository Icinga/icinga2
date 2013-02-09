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

#include "i2-icinga.h"

using namespace icinga;

map<String, set<Notification::WeakPtr> > Service::m_NotificationsCache;
bool Service::m_NotificationsCacheValid;

void Service::SendNotifications(void) const
{
	BOOST_FOREACH(const Notification::Ptr& notification, GetNotifications()) {
		notification->SendNotification();
	}
}

void Service::InvalidateNotificationsCache(void)
{
	m_NotificationsCacheValid = false;
	m_NotificationsCache.clear();
}

void Service::ValidateNotificationsCache(void)
{
	if (m_NotificationsCacheValid)
		return;

	m_NotificationsCache.clear();

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicType::GetByName("Notification")->GetObjects()) {
		const Notification::Ptr& notification = static_pointer_cast<Notification>(object);

		m_NotificationsCache[notification->GetService()->GetName()].insert(notification);
	}

	m_NotificationsCacheValid = true;
}

set<Notification::Ptr> Service::GetNotifications(void) const
{
	set<Notification::Ptr> notifications;

	ValidateNotificationsCache();

	BOOST_FOREACH(const Notification::WeakPtr& wservice, m_NotificationsCache[GetName()]) {
		Notification::Ptr notification = wservice.lock();

		if (!notification)
			continue;

		notifications.insert(notification);
	}

	return notifications;
}
