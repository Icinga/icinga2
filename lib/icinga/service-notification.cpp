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

boost::mutex Service::m_NotificationMutex;
map<String, set<Notification::WeakPtr> > Service::m_NotificationsCache;
bool Service::m_NotificationsCacheValid = true;

void Service::RequestNotifications(NotificationType type)
{
	RequestMessage msg;
	msg.SetMethod("icinga::SendNotifications");

	NotificationRequestMessage params;
	msg.SetParams(params);

	params.SetService(GetName());
	params.SetType(type);

	Logger::Write(LogDebug, "icinga", "Sending notification anycast request for service '" + GetName() + "'");
	EndpointManager::GetInstance()->SendAnycastMessage(Endpoint::Ptr(), msg);

	SetLastNotification(Utility::GetTime());
}

void Service::SendNotifications(const Service::Ptr& self, NotificationType type)
{
	String service_name;
	bool enable_notifications;

	{
		ObjectLock olock(self);
		service_name = self->GetName();
		enable_notifications = self->GetEnableNotifications();
	}

	if (!enable_notifications) {
		Logger::Write(LogInformation, "icinga", "Notifications are disabled for service '" + service_name + "'.");
		return;
	}

	Logger::Write(LogInformation, "icinga", "Sending notifications for service '" + service_name + "'");

	set<Notification::Ptr> notifications = GetNotifications(self);

	if (notifications.size() == 0)
		Logger::Write(LogInformation, "icinga", "Service '" + service_name + "' does not have any notifications.");

	BOOST_FOREACH(const Notification::Ptr& notification, notifications) {
		try {
			Notification::BeginExecuteNotification(notification, type);
		} catch (const exception& ex) {
			stringstream msgbuf;
			msgbuf << "Exception occured during notification for service '"
			       << service_name << "': " << diagnostic_information(ex);
			String message = msgbuf.str();

			Logger::Write(LogWarning, "icinga", message);
		}
	}
}

void Service::InvalidateNotificationsCache(void)
{
	{
		boost::mutex::scoped_lock lock(m_NotificationMutex);
		m_NotificationsCacheValid = false;
	}

	Utility::QueueAsyncCallback(boost::bind(&Service::RefreshNotificationsCache));
}

void Service::RefreshNotificationsCache(void)
{
	{
		boost::mutex::scoped_lock lock(m_NotificationMutex);

		if (m_NotificationsCacheValid)
			return;

		m_NotificationsCacheValid = true;
	}

	map<String, set<Notification::WeakPtr> > newNotificationsCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Notification")) {
		const Notification::Ptr& notification = static_pointer_cast<Notification>(object);

		Service::Ptr service;

		{
			ObjectLock olock(notification);
			service = notification->GetService();
		}

		String service_name;

		{
			ObjectLock olock(service);
			service_name = service->GetName();
		}

		newNotificationsCache[service_name].insert(notification);
	}

	boost::mutex::scoped_lock lock(m_NotificationMutex);
	m_NotificationsCache.swap(newNotificationsCache);
}

set<Notification::Ptr> Service::GetNotifications(const Service::Ptr& self)
{
	String name;

	{
		ObjectLock olock(self);
		name = self->GetName();
	}

	set<Notification::Ptr> notifications;

	{
		boost::mutex::scoped_lock lock(m_NotificationMutex);

		BOOST_FOREACH(const Notification::WeakPtr& wservice, m_NotificationsCache[name]) {
			Notification::Ptr notification = wservice.lock();

			if (!notification)
				continue;

			notifications.insert(notification);
		}
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

	Value users = notificationDesc->Get("users");
	if (!users.IsEmpty())
		builder->AddExpression("users", OperatorPlus, users);

	/*Value notificationInterval = notificationDesc->Get("notification_interval");
	if (!notificationInterval.IsEmpty())
		builder->AddExpression("notification_interval", OperatorSet, notificationInterval);*/
}

void Service::UpdateSlaveNotifications(const Service::Ptr& self)
{
	Dictionary::Ptr oldNotifications;
	Host::Ptr host;
	vector<Dictionary::Ptr> notificationDescsList;
	String service_name, short_name;
	ConfigItem::Ptr item;

	{
		ObjectLock olock(self);

		item = ConfigItem::GetObject("Service", self->GetName());

		/* Don't create slave notifications unless we own this object
		 * and it's not a template. */
		if (!item || self->IsAbstract())
			return;

		service_name = self->GetName();
		short_name = self->GetShortName();
		oldNotifications = self->m_SlaveNotifications;
		host = self->GetHost();

		notificationDescsList.push_back(self->Get("notifications"));
	}

	DebugInfo debug_info;

	{
		ObjectLock ilock(item);
		debug_info = item->GetDebugInfo();
	}

	Dictionary::Ptr newNotifications;
	newNotifications = boost::make_shared<Dictionary>();

	String host_name;

	{
		ObjectLock olock(host);

		notificationDescsList.push_back(host->Get("notifications"));
		host_name = host->GetName();
	}

	BOOST_FOREACH(const Dictionary::Ptr& notificationDescs, notificationDescsList) {
		if (!notificationDescs)
			continue;

		ObjectLock olock(notificationDescs);

		String nfcname;
		Value nfcdesc;
		BOOST_FOREACH(tie(nfcname, nfcdesc), notificationDescs) {
			if (nfcdesc.IsScalar())
				nfcname = nfcdesc;

			stringstream namebuf;
			namebuf << service_name << "-" << nfcname;
			String name = namebuf.str();

			ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>(debug_info);
			builder->SetType("Notification");
			builder->SetName(name);
			builder->AddExpression("host_name", OperatorSet, host_name);
			builder->AddExpression("service", OperatorSet, short_name);

			CopyNotificationAttributes(self, builder);

			if (nfcdesc.IsScalar()) {
				builder->AddParent(nfcdesc);
			} else if (nfcdesc.IsObjectType<Dictionary>()) {
				Dictionary::Ptr notification = nfcdesc;
				ObjectLock nlock(notification);

				Dictionary::Ptr templates = notification->Get("templates");

				if (templates) {
					String tmpl;
					BOOST_FOREACH(tie(tuples::ignore, tmpl), templates) {
						builder->AddParent(tmpl);
					}
				} else {
					builder->AddParent(nfcname);
				}

				CopyNotificationAttributes(notification, builder);
			} else {
				BOOST_THROW_EXCEPTION(invalid_argument("Notification description must be either a string or a dictionary."));
			}

			ConfigItem::Ptr notificationItem = builder->Compile();
			ConfigItem::Commit(notificationItem);

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

	{
		ObjectLock olock(self);
		self->m_SlaveNotifications = newNotifications;
	}
}

double Service::GetLastNotification(void) const
{
	if (m_LastNotification.IsEmpty())
		return 0;
	else
		return m_LastNotification;
}

void Service::SetLastNotification(double time)
{
	m_LastNotification = time;
	Touch("last_notification");
}

bool Service::GetEnableNotifications(void) const
{
	if (m_EnableNotifications.IsEmpty())
		return true;
	else
		return m_EnableNotifications;
}

void Service::SetEnableNotifications(bool enabled)
{
	m_EnableNotifications = enabled;
	Touch("enable_notifications");
}

double Service::GetNotificationInterval(void) const
{
	if (m_NotificationInterval.IsEmpty())
		return 300;
	else
		return m_NotificationInterval;
}
