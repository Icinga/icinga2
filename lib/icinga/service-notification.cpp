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

#include "icinga/service.h"
#include "icinga/icingaapplication.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/timer.h"
#include "base/utility.h"
#include "config/configitembuilder.h"
#include <boost/tuple/tuple.hpp>
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/exception/diagnostic_information.hpp>

using namespace icinga;

boost::signals2::signal<void (const Service::Ptr&, const std::set<User::Ptr>&, const NotificationType&, const Dictionary::Ptr&, const String&, const String&)> Service::OnNotificationSentToAllUsers;
boost::signals2::signal<void (const Service::Ptr&, const User::Ptr&, const NotificationType&, const Dictionary::Ptr&, const String&, const String&)> Service::OnNotificationSentToUser;

Dictionary::Ptr Service::GetNotificationDescriptions(void) const
{
	return m_NotificationDescriptions;
}

void Service::ResetNotificationNumbers(void)
{
	BOOST_FOREACH(const Notification::Ptr& notification, GetNotifications()) {
		ObjectLock olock(notification);
		notification->ResetNotificationNumber();
	}
}

void Service::SendNotifications(NotificationType type, const Dictionary::Ptr& cr, const String& author, const String& text)
{
	bool force = GetForceNextNotification();

	if (!IcingaApplication::GetInstance()->GetEnableNotifications() || !GetEnableNotifications()) {
		if (!force) {
			Log(LogInformation, "icinga", "Notifications are disabled for service '" + GetName() + "'.");
			return;
		}

		SetForceNextNotification(false);
	}

	Log(LogInformation, "icinga", "Sending notifications for service '" + GetName() + "'");

	std::set<Notification::Ptr> notifications = GetNotifications();

	if (notifications.empty())
		Log(LogInformation, "icinga", "Service '" + GetName() + "' does not have any notifications.");

	BOOST_FOREACH(const Notification::Ptr& notification, notifications) {
		try {
			notification->BeginExecuteNotification(type, cr, force, author, text);
		} catch (const std::exception& ex) {
			std::ostringstream msgbuf;
			msgbuf << "Exception occured during notification for service '"
			       << GetName() << "': " << boost::diagnostic_information(ex);
			String message = msgbuf.str();

			Log(LogWarning, "icinga", message);
		}
	}
}

std::set<Notification::Ptr> Service::GetNotifications(void) const
{
	return m_Notifications;
}

void Service::AddNotification(const Notification::Ptr& notification)
{
	m_Notifications.insert(notification);
}

void Service::RemoveNotification(const Notification::Ptr& notification)
{
	m_Notifications.erase(notification);
}

void Service::UpdateSlaveNotifications(void)
{
	ConfigItem::Ptr item = ConfigItem::GetObject("Service", GetName());

	/* Don't create slave notifications unless we own this object */
	if (!item)
		return;

	/* Service notification descs */
	Dictionary::Ptr descs = GetNotificationDescriptions();

	if (!descs)
		return;

	ObjectLock olock(descs);

	String nfcname;
	Value nfcdesc;
	BOOST_FOREACH(boost::tie(nfcname, nfcdesc), descs) {
		std::ostringstream namebuf;
		namebuf << GetName() << ":" << nfcname;
		String name = namebuf.str();

		std::vector<String> path;
		path.push_back("notifications");
		path.push_back(nfcname);

		DebugInfo di;
		item->GetLinkedExpressionList()->FindDebugInfoPath(path, di);

		if (di.Path.IsEmpty())
			di = item->GetDebugInfo();

		ConfigItemBuilder::Ptr builder = boost::make_shared<ConfigItemBuilder>(di);
		builder->SetType("Notification");
		builder->SetName(name);
		builder->AddExpression("host", OperatorSet, GetHost()->GetName());
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

		/* Clone attributes from the notification expression list. */
		ExpressionList::Ptr nfc_exprl = boost::make_shared<ExpressionList>();
		item->GetLinkedExpressionList()->ExtractPath(path, nfc_exprl);

		std::vector<String> dpath;
		dpath.push_back("templates");
		nfc_exprl->ErasePath(dpath);

		builder->AddExpressionList(nfc_exprl);

		ConfigItem::Ptr notificationItem = builder->Compile();
		notificationItem->Register();
		DynamicObject::Ptr dobj = notificationItem->Commit();
		dobj->OnConfigLoaded();
	}
}

bool Service::GetEnableNotifications(void) const
{
	if (m_EnableNotifications.IsEmpty())
		return true;
	else
		return m_EnableNotifications;
}

void Service::SetEnableNotifications(bool enabled, const String& authority)
{
	m_EnableNotifications = enabled;

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnEnableNotificationsChanged), GetSelf(), enabled, authority));
}

bool Service::GetForceNextNotification(void) const
{
	if (m_ForceNextNotification.IsEmpty())
		return false;

	return static_cast<bool>(m_ForceNextNotification);
}

void Service::SetForceNextNotification(bool forced, const String& authority)
{
	m_ForceNextNotification = forced ? 1 : 0;

	Utility::QueueAsyncCallback(boost::bind(boost::ref(OnForceNextNotificationChanged), GetSelf(), forced, authority));
}
