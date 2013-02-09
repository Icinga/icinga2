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

void Service::RequestNotifications(NotificationType type) const
{
	RequestMessage msg;
	msg.SetMethod("icinga::SendNotifications");

	NotificationRequestMessage params;
	msg.SetParams(params);

	params.SetService(GetName());
	params.SetType(type);

	Logger::Write(LogInformation, "icinga", "Sending notification anycast request for service '" + GetName() + "'");
	EndpointManager::GetInstance()->SendAnycastMessage(Endpoint::Ptr(), msg);
}

void Service::SendNotifications(NotificationType type)
{
	Logger::Write(LogInformation, "icinga", "Sending notifications for service '" + GetName() + "'");

	set<Notification::Ptr> notifications = GetNotifications();

	if (notifications.size() == 0)
		Logger::Write(LogInformation, "icinga", "Service '" + GetName() + "' does not have any notifications.");

	BOOST_FOREACH(const Notification::Ptr& notification, notifications) {
		notification->SendNotification(type);
	}

	SetLastNotification(Utility::GetTime());
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

template<typename TDict>
static void CopyNotificationAttributes(TDict notificationDesc, const ConfigItemBuilder::Ptr& builder)
{
	/* TODO: we only need to copy macros if this is an inline definition,
	 * i.e. "typeid(notificationDesc)" != Notification, however for now we just
	 * copy them anyway. */
	Value macros = notificationDesc->Get("macros");
	if (!macros.IsEmpty())
		builder->AddExpression("macros", OperatorPlus, macros);

	/*Value notificationInterval = notificationDesc->Get("notification_interval");
	if (!notificationInterval.IsEmpty())
		builder->AddExpression("notification_interval", OperatorSet, notificationInterval);*/
}

void Service::UpdateSlaveNotifications(void)
{
	ConfigItem::Ptr item = ConfigItem::GetObject("Service", GetName());

	/* Don't create slave notifications unless we own this object
	 * and it's not a template. */
	if (!item || IsAbstract())
		return;

	Dictionary::Ptr oldNotifications = Get("slave_notifications");

	Dictionary::Ptr newNotifications;
	newNotifications = boost::make_shared<Dictionary>();

	vector<Dictionary::Ptr> notificationDescsList;
	notificationDescsList.push_back(GetHost()->Get("notifications"));
	notificationDescsList.push_back(Get("notifications"));

	BOOST_FOREACH(const Dictionary::Ptr& notificationDescs, notificationDescsList) {
		if (!notificationDescs)
			continue;

		String nfcname;
		Value nfcdesc;
		BOOST_FOREACH(tie(nfcname, nfcdesc), notificationDescs) {
			stringstream namebuf;
			namebuf << GetName() << "-" << nfcname;
			String name = namebuf.str();

			ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>(item->GetDebugInfo());
			builder->SetType("Notification");
			builder->SetName(name);
			builder->AddExpression("host_name", OperatorSet, GetHost()->GetName());
			builder->AddExpression("service", OperatorSet, GetName());

			CopyNotificationAttributes(this, builder);

			if (nfcdesc.IsScalar()) {
				builder->AddParent(nfcdesc);
			} else if (nfcdesc.IsObjectType<Dictionary>()) {
				Dictionary::Ptr notification = nfcdesc;

				String parent = notification->Get("notification");
				if (parent.IsEmpty())
					parent = nfcname;

				builder->AddParent(parent);

				CopyNotificationAttributes(notification, builder);
			} else {
				BOOST_THROW_EXCEPTION(invalid_argument("Notification description must be either a string or a dictionary."));
			}

			ConfigItem::Ptr notificationItem = builder->Compile();
			notificationItem->Commit();

			newNotifications->Set(name, notificationItem);
		}
	}

	if (oldNotifications) {
		ConfigItem::Ptr notification;
		BOOST_FOREACH(tie(tuples::ignore, notification), oldNotifications) {
			if (!notification)
				continue;

			if (!newNotifications->Contains(notification->GetName()))
				notification->Unregister();
		}
	}

	Set("slave_notifications", newNotifications);
}

double Service::GetLastNotification(void) const
{
	Value value = Get("last_notification");

	if (value.IsEmpty())
		value = 0;

	return value;
}

void Service::SetLastNotification(double time)
{
	Set("last_notification", time);
}

double Service::GetNextNotification(void) const
{
	Value value = Get("next_notification");

	if (value.IsEmpty())
		value = 0;

	return value;
}

void Service::SetNextNotification(double time)
{
	Set("next_notification", time);
}
