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

REGISTER_TYPE(Notification);

Notification::Notification(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	RegisterAttribute("notification_command", Attribute_Config, &m_NotificationCommand);
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

/**
 * @threadsafety Always.
 */
Notification::Ptr Notification::GetByName(const String& name)
{
	DynamicObject::Ptr configObject = DynamicObject::GetObject("Notification", name);

	return dynamic_pointer_cast<Notification>(configObject);
}

/**
 * @threadsafety Always.
 */
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

/**
 * @threadsafety Always.
 */
Value Notification::GetNotificationCommand(void) const
{
	return m_NotificationCommand;
}

/**
 * @threadsafety Always.
 */
Dictionary::Ptr Notification::GetMacros(void) const
{
	return m_Macros;
}

/**
 * @threadsafety Always.
 */
set<User::Ptr> Notification::GetUsers(void) const
{
	set<User::Ptr> result;

	Dictionary::Ptr users = m_Users;

	if (users) {
		ObjectLock olock(users);

		String name;
		BOOST_FOREACH(tie(tuples::ignore, name), users) {
			User::Ptr user = User::GetByName(name);

			if (!user)
				continue;

			result.insert(user);
		}
	}

	return result;
}

/**
 * @threadsafety Always.
 */
set<UserGroup::Ptr> Notification::GetGroups(void) const
{
	set<UserGroup::Ptr> result;

	Dictionary::Ptr groups = m_Groups;

	if (groups) {
		ObjectLock olock(groups);

		String name;
		BOOST_FOREACH(tie(tuples::ignore, name), groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (!ug)
				continue;

			result.insert(ug);
		}
	}

	return result;
}

/**
 * @threadsafety Always.
 */
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

/**
 * @threadsafety Always.
 */
void Notification::BeginExecuteNotification(NotificationType type)
{
	assert(!OwnsLock());

	vector<Dictionary::Ptr> macroDicts;

	Dictionary::Ptr notificationMacros = boost::make_shared<Dictionary>();
	notificationMacros->Set("NOTIFICATIONTYPE", NotificationTypeToString(type));
	macroDicts.push_back(notificationMacros);

	macroDicts.push_back(GetMacros());

	Service::Ptr service = GetService();

	if (service) {
		macroDicts.push_back(service->GetMacros());
		macroDicts.push_back(service->CalculateDynamicMacros());

		Host::Ptr host = service->GetHost();

		if (host) {
			macroDicts.push_back(host->GetMacros());
			macroDicts.push_back(host->CalculateDynamicMacros());
		}
	}

	IcingaApplication::Ptr app = IcingaApplication::GetInstance();
	macroDicts.push_back(app->GetMacros());

	macroDicts.push_back(IcingaApplication::CalculateDynamicMacros());

	Dictionary::Ptr macros = MacroProcessor::MergeMacroDicts(macroDicts);

	set<User::Ptr> allUsers;

	set<User::Ptr> users = GetUsers();
	std::copy(users.begin(), users.end(), std::inserter(allUsers, allUsers.begin()));

	BOOST_FOREACH(const UserGroup::Ptr& ug, GetGroups()) {
		set<User::Ptr> members = ug->GetMembers();
		std::copy(members.begin(), members.end(), std::inserter(allUsers, allUsers.begin()));
	}

	BOOST_FOREACH(const User::Ptr& user, allUsers) {
		Logger::Write(LogDebug, "icinga", "Sending notification for user " + user->GetName());
		BeginExecuteNotificationHelper(macros, type, user);
	}

	if (allUsers.empty()) {
		/* Send a notification even if there are no users specified. */
		BeginExecuteNotificationHelper(macros, type, User::Ptr());
	}
}

/**
 * @threadsafety Always.
 */
void Notification::BeginExecuteNotificationHelper(const Dictionary::Ptr& notificationMacros, NotificationType type, const User::Ptr& user)
{
	assert(!OwnsLock());

	vector<Dictionary::Ptr> macroDicts;

	if (user) {
		macroDicts.push_back(user->GetMacros());
		macroDicts.push_back(user->CalculateDynamicMacros());
	}

	macroDicts.push_back(notificationMacros);

	Dictionary::Ptr macros = MacroProcessor::MergeMacroDicts(macroDicts);

	Notification::Ptr self = GetSelf();

	vector<Value> arguments;
	arguments.push_back(self);
	arguments.push_back(macros);
	arguments.push_back(type);

	ScriptTask::Ptr task;
	task = MakeMethodTask("notify", arguments);

	if (!task) {
		Logger::Write(LogWarning, "icinga", "Notification object '" + GetName() + "' doesn't have a 'notify' method.");

		return;
	}

	{
		ObjectLock olock(this);

		/* We need to keep the task object alive until the completion handler is called. */
		m_Tasks.insert(task);
	}

	task->Start(boost::bind(&Notification::NotificationCompletedHandler, self, _1));
}

/**
 * @threadsafety Always.
 */
void Notification::NotificationCompletedHandler(const ScriptTask::Ptr& task)
{
	assert(!OwnsLock());

	{
		ObjectLock olock(this);

		m_Tasks.erase(task);
	}

	try {
		task->GetResult();

		Logger::Write(LogInformation, "icinga", "Completed sending notification for service '" + GetService()->GetName() + "'");
	} catch (const exception& ex) {
		stringstream msgbuf;
		msgbuf << "Exception occured during notification for service '"
		       << GetService()->GetName() << "': " << diagnostic_information(ex);
		String message = msgbuf.str();

		Logger::Write(LogWarning, "icinga", message);
	}
}

/**
 * @threadsafety Always.
 */
void Notification::OnAttributeChanged(const String& name)
{
	assert(!OwnsLock());

	if (name == "host_name" || name == "service")
		Service::InvalidateNotificationsCache();
}
