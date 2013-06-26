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

#include "icinga/service.h"
#include "icinga/notificationrequestmessage.h"
#include "remoting/endpointmanager.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "config/configitembuilder.h"
#include <boost/tuple/tuple.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/exception/diagnostic_information.hpp>

using namespace icinga;

static boost::mutex l_NotificationMutex;
static std::map<String, std::set<Notification::WeakPtr> > l_NotificationsCache;
static bool l_NotificationsCacheNeedsUpdate = false;
static Timer::Ptr l_NotificationsCacheTimer;

void Service::RequestNotifications(NotificationType type, const Dictionary::Ptr& cr)
{
	RequestMessage msg;
	msg.SetMethod("icinga::SendNotifications");

	NotificationRequestMessage params;
	msg.SetParams(params);

	params.SetService(GetName());
	params.SetType(type);
	params.SetCheckResult(cr);

	Log(LogDebug, "icinga", "Sending notification anycast request for service '" + GetName() + "'");
	EndpointManager::GetInstance()->SendAnycastMessage(Endpoint::Ptr(), msg);
}

void Service::SendNotifications(NotificationType type, const Dictionary::Ptr& cr)
{
	bool force = false;

	if (!GetEnableNotifications()) {
		if (!GetForceNextNotification()) {
			Log(LogInformation, "icinga", "Notifications are disabled for service '" + GetName() + "'.");
			return;
		}

		force = true;
		SetForceNextNotification(false);
	}

	Log(LogInformation, "icinga", "Sending notifications for service '" + GetName() + "'");

	std::set<Notification::Ptr> notifications = GetNotifications();

	if (notifications.empty())
		Log(LogInformation, "icinga", "Service '" + GetName() + "' does not have any notifications.");

	BOOST_FOREACH(const Notification::Ptr& notification, notifications) {
		try {
			notification->BeginExecuteNotification(type, cr, force);
		} catch (const std::exception& ex) {
			std::ostringstream msgbuf;
			msgbuf << "Exception occured during notification for service '"
			       << GetName() << "': " << boost::diagnostic_information(ex);
			String message = msgbuf.str();

			Log(LogWarning, "icinga", message);
		}
	}
}

void Service::InvalidateNotificationsCache(void)
{
	boost::mutex::scoped_lock lock(l_NotificationMutex);

	if (l_NotificationsCacheNeedsUpdate)
		return; /* Someone else has already requested a refresh. */

	if (!l_NotificationsCacheTimer) {
		l_NotificationsCacheTimer = boost::make_shared<Timer>();
		l_NotificationsCacheTimer->SetInterval(0.5);
		l_NotificationsCacheTimer->OnTimerExpired.connect(boost::bind(&Service::RefreshNotificationsCache));
		l_NotificationsCacheTimer->Start();
	}

	l_NotificationsCacheNeedsUpdate = true;
}

void Service::RefreshNotificationsCache(void)
{
	{
		boost::mutex::scoped_lock lock(l_NotificationMutex);

		if (!l_NotificationsCacheNeedsUpdate)
			return;

		l_NotificationsCacheNeedsUpdate = false;
	}

	Log(LogDebug, "icinga", "Updating Service notifications cache.");

	std::map<String, std::set<Notification::WeakPtr> > newNotificationsCache;

	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("Notification")) {
		const Notification::Ptr& notification = static_pointer_cast<Notification>(object);

		Service::Ptr service = notification->GetService();

		if (!service)
			continue;

		newNotificationsCache[service->GetName()].insert(notification);
	}

	boost::mutex::scoped_lock lock(l_NotificationMutex);
	l_NotificationsCache.swap(newNotificationsCache);
}

std::set<Notification::Ptr> Service::GetNotifications(void) const
{
	std::set<Notification::Ptr> notifications;

	{
		boost::mutex::scoped_lock lock(l_NotificationMutex);

		BOOST_FOREACH(const Notification::WeakPtr& wservice, l_NotificationsCache[GetName()]) {
			Notification::Ptr notification = wservice.lock();

			if (!notification)
				continue;

			notifications.insert(notification);
		}
	}

	return notifications;
}

void Service::UpdateSlaveNotifications(void)
{
	Dictionary::Ptr oldNotifications;
	ConfigItem::Ptr serviceItem, hostItem;

	serviceItem = ConfigItem::GetObject("Service", GetName());

	/* Don't create slave notifications unless we own this object */
	if (!serviceItem)
		return;

	Host::Ptr host = GetHost();

	if (!host)
		return;

	hostItem = ConfigItem::GetObject("Host", host->GetName());

	/* Don't create slave notifications unless we own the host */
	if (!hostItem)
		return;

	{
		ObjectLock olock(this);
		oldNotifications = m_SlaveNotifications;
	}

	std::vector<Dictionary::Ptr> descLists;

	descLists.push_back(Get("notifications"));

	Dictionary::Ptr newNotifications;
	newNotifications = boost::make_shared<Dictionary>();

	descLists.push_back(host->Get("notifications"));

	for (int i = 0; i < 2; i++) {
		Dictionary::Ptr descs;
		ConfigItem::Ptr item;

		if (i == 0) {
			/* Host notification descs */
			descs = host->Get("notifications");
			item = hostItem;
		} else {
			/* Service notification descs */
			descs = Get("notifications");
			item = serviceItem;
		}

		if (!descs)
			continue;

		ObjectLock olock(descs);

		String nfcname;
		Value nfcdesc;
		BOOST_FOREACH(boost::tie(nfcname, nfcdesc), descs) {
			std::ostringstream namebuf;
			namebuf << GetName() << ":" << nfcname;
			String name = namebuf.str();

			ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>(item->GetDebugInfo());
			builder->SetType("Notification");
			builder->SetName(name);
			builder->AddExpression("host_name", OperatorSet, host->GetName());
			builder->AddExpression("service", OperatorSet, GetShortName());

			if (!nfcdesc.IsObjectType<Dictionary>())
				BOOST_THROW_EXCEPTION(std::invalid_argument("Notification description must be a dictionary."));

			Dictionary::Ptr notification = nfcdesc;

			Array::Ptr templates = notification->Get("templates");

			if (templates) {
				ObjectLock tlock(templates);

				BOOST_FOREACH(const Value& tmpl, templates) {
					builder->AddParent(tmpl);
				}
			}

			/* Clone attributes from the host/service object. */
			std::set<String, string_iless> keys;
			keys.insert("users");
			keys.insert("groups");
			keys.insert("notification_interval");
			keys.insert("notification_period");
			keys.insert("notification_type_filter");
			keys.insert("notification_state_filter");
			keys.insert("export_macros");

			ExpressionList::Ptr svc_exprl = boost::make_shared<ExpressionList>();
			item->GetLinkedExpressionList()->ExtractFiltered(keys, svc_exprl);
			builder->AddExpressionList(svc_exprl);

			/* Clone attributes from the notification expression list. */
			std::vector<String> path;
			path.push_back("notifications");
			path.push_back(nfcname);

			ExpressionList::Ptr nfc_exprl = boost::make_shared<ExpressionList>();
			item->GetLinkedExpressionList()->ExtractPath(path, nfc_exprl);
			builder->AddExpressionList(nfc_exprl);

			ConfigItem::Ptr notificationItem = builder->Compile();
			notificationItem->Commit();

			newNotifications->Set(name, notificationItem);
		}
	}

	if (oldNotifications) {
		ObjectLock olock(oldNotifications);

		ConfigItem::Ptr notification;
		BOOST_FOREACH(boost::tie(boost::tuples::ignore, notification), oldNotifications) {
			if (!notification)
				continue;

			if (!newNotifications->Contains(notification->GetName()))
				notification->Unregister();
		}
	}

	{
		ObjectLock olock(this);
		m_SlaveNotifications = newNotifications;
	}
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

bool Service::GetForceNextNotification(void) const
{
	if (m_ForceNextNotification.IsEmpty())
		return false;

	return static_cast<bool>(m_ForceNextNotification);
}

void Service::SetForceNextNotification(bool forced)
{
	m_ForceNextNotification = forced ? 1 : 0;
	Touch("force_next_notification");
}
