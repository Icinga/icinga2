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

#include "icinga/notification.h"
#include "icinga/macroprocessor.h"
#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/utility.h"
#include <boost/tuple/tuple.hpp>
#include <boost/foreach.hpp>
#include <boost/exception/diagnostic_information.hpp>

using namespace icinga;

REGISTER_TYPE(Notification);

Notification::Notification(const Dictionary::Ptr& serializedUpdate)
	: DynamicObject(serializedUpdate)
{
	RegisterAttribute("notification_command", Attribute_Config, &m_NotificationCommand);
	RegisterAttribute("notification_interval", Attribute_Config, &m_NotificationInterval);
	RegisterAttribute("notification_period", Attribute_Config, &m_NotificationPeriod);
	RegisterAttribute("last_notification", Attribute_Replicated, &m_LastNotification);
	RegisterAttribute("next_notification", Attribute_Replicated, &m_NextNotification);
	RegisterAttribute("macros", Attribute_Config, &m_Macros);
	RegisterAttribute("users", Attribute_Config, &m_Users);
	RegisterAttribute("groups", Attribute_Config, &m_Groups);
	RegisterAttribute("host_name", Attribute_Config, &m_HostName);
	RegisterAttribute("service", Attribute_Config, &m_Service);
}

Notification::~Notification(void)
{
	Service::InvalidateNotificationsCache();
}

Notification::Ptr Notification::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("Notification", name);

	return dynamic_pointer_cast<Notification>(configObject);
}

Service::Ptr Notification::GetService(void) const
{
	Host::Ptr host = Host::GetByName(m_HostName);

	if (!host)
		return Service::Ptr();

	if (m_Service.IsEmpty())
		return host->GetHostCheckService();
	else
		return host->GetServiceByShortName(m_Service);
}

Value Notification::GetNotificationCommand(void) const
{
	return m_NotificationCommand;
}

Dictionary::Ptr Notification::GetMacros(void) const
{
	return m_Macros;
}

Array::Ptr Notification::GetExportMacros(void) const
{
	return m_ExportMacros;
}

std::set<User::Ptr> Notification::GetUsers(void) const
{
	std::set<User::Ptr> result;

	Array::Ptr users = m_Users;

	if (users) {
		ObjectLock olock(users);

		BOOST_FOREACH(const String& name, users) {
			User::Ptr user = User::GetByName(name);

			if (!user)
				continue;

			result.insert(user);
		}
	}

	return result;
}

std::set<UserGroup::Ptr> Notification::GetGroups(void) const
{
	std::set<UserGroup::Ptr> result;

	Array::Ptr groups = m_Groups;

	if (groups) {
		ObjectLock olock(groups);

		BOOST_FOREACH(const String& name, groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (!ug)
				continue;

			result.insert(ug);
		}
	}

	return result;
}

double Notification::GetNotificationInterval(void) const
{
	if (m_NotificationInterval.IsEmpty())
		return 300;
	else
		return m_NotificationInterval;
}

TimePeriod::Ptr Notification::GetNotificationPeriod(void) const
{
	return TimePeriod::GetByName(m_NotificationPeriod);
}

double Notification::GetLastNotification(void) const
{
	if (m_LastNotification.IsEmpty())
		return 0;
	else
		return m_LastNotification;
}

/**
 * Sets the timestamp when the last notification was sent.
 */
void Notification::SetLastNotification(double time)
{
	m_LastNotification = time;
	Touch("last_notification");
}

double Notification::GetNextNotification(void) const
{
	if (m_NextNotification.IsEmpty())
		return 0;
	else
		return m_NextNotification;
}

/**
 * Sets the timestamp when the next periodical notification should be sent.
 * This does not affect notifications that are sent for state changes.
 */
void Notification::SetNextNotification(double time)
{
	m_NextNotification = time;
	Touch("next_notification");
}

String Notification::NotificationTypeToString(NotificationType type)
{
	switch (type) {
		case NotificationDowntimeStart:
			return "DOWNTIMESTART";
		case NotificationDowntimeEnd:
			return "DOWNTIMEEND";
		case NotificationDowntimeRemoved:
			return "DOWNTIMECANCELLED";
		case NotificationCustom:
			return "CUSTOM";
		case NotificationAcknowledgement:
			return "ACKNOWLEDGEMENT";
		case NotificationProblem:
			return "PROBLEM";
		case NotificationRecovery:
			return "RECOVERY";
		default:
			return "UNKNOWN_NOTIFICATION";
	}
}

void Notification::BeginExecuteNotification(NotificationType type, const Dictionary::Ptr& cr, bool ignore_timeperiod)
{
	ASSERT(!OwnsLock());

	if (!ignore_timeperiod) {
		TimePeriod::Ptr tp = GetNotificationPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogInformation, "icinga", "Not sending notifications for notification object '" + GetName() + "': not in timeperiod");
			return;
		}
	}

	{
		ObjectLock olock(this);

		SetLastNotification(Utility::GetTime());
	}

	std::set<User::Ptr> allUsers;

	std::set<User::Ptr> users = GetUsers();
	std::copy(users.begin(), users.end(), std::inserter(allUsers, allUsers.begin()));

	BOOST_FOREACH(const UserGroup::Ptr& ug, GetGroups()) {
		std::set<User::Ptr> members = ug->GetMembers();
		std::copy(members.begin(), members.end(), std::inserter(allUsers, allUsers.begin()));
	}

	BOOST_FOREACH(const User::Ptr& user, allUsers) {
		Log(LogDebug, "icinga", "Sending notification for user " + user->GetName());
		Utility::QueueAsyncCallback(boost::bind(&Notification::ExecuteNotificationHelper, this, type, user, cr, ignore_timeperiod));
	}
}

void Notification::ExecuteNotificationHelper(NotificationType type, const User::Ptr& user, const Dictionary::Ptr& cr, bool ignore_timeperiod)
{
	ASSERT(!OwnsLock());

	if (!ignore_timeperiod) {
		TimePeriod::Ptr tp = user->GetNotificationPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogInformation, "icinga", "Not sending notifications for notification object '" +
			    GetName() + " and user '" + user->GetName() + "': user not in timeperiod");
			return;
		}
	}

	Notification::Ptr self = GetSelf();

	std::vector<Value> arguments;
	arguments.push_back(self);
	arguments.push_back(user);
	arguments.push_back(cr);
	arguments.push_back(type);

	try {
		InvokeMethod("notify", arguments);

		Log(LogInformation, "icinga", "Completed sending notification for service '" + GetService()->GetName() + "'");
	} catch (const std::exception& ex) {
		std::ostringstream msgbuf;
		msgbuf << "Exception occured during notification for service '"
		       << GetService()->GetName() << "': " << boost::diagnostic_information(ex);
		String message = msgbuf.str();

		Log(LogWarning, "icinga", message);
	}
}

void Notification::OnAttributeChanged(const String& name)
{
	ASSERT(!OwnsLock());

	if (name == "host_name" || name == "service")
		Service::InvalidateNotificationsCache();
}

bool Notification::ResolveMacro(const String& macro, const Dictionary::Ptr& cr, String *result) const
{
	Dictionary::Ptr macros = GetMacros();

	if (macros && macros->Contains(macro)) {
		*result = macros->Get(macro);
		return true;
	}

	return false;
}
