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

#include "icinga/notification.h"
#include "icinga/notificationcommand.h"
#include "icinga/macroprocessor.h"
#include "icinga/service.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/logger_fwd.h"
#include "base/utility.h"
#include "base/convert.h"
#include "base/exception.h"
#include "base/initialize.h"
#include "base/scriptvariable.h"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Notification);
INITIALIZE_ONCE(&Notification::StaticInitialize);

boost::signals2::signal<void (const Notification::Ptr&, double, const String&)> Notification::OnNextNotificationChanged;

void Notification::StaticInitialize(void)
{
	ScriptVariable::Set("NotificationDowntimeStart", NotificationDowntimeStart, true, true);
	ScriptVariable::Set("NotificationDowntimeEnd", NotificationDowntimeEnd, true, true);
	ScriptVariable::Set("NotificationDowntimeRemoved", NotificationDowntimeRemoved, true, true);
	ScriptVariable::Set("NotificationCustom", NotificationCustom, true, true);
	ScriptVariable::Set("NotificationAcknowledgement", NotificationAcknowledgement, true, true);
	ScriptVariable::Set("NotificationProblem", NotificationProblem, true, true);
	ScriptVariable::Set("NotificationRecovery", NotificationRecovery, true, true);
	ScriptVariable::Set("NotificationFlappingStart", NotificationFlappingStart, true, true);
	ScriptVariable::Set("NotificationFlappingEnd", NotificationFlappingEnd, true, true);

	ScriptVariable::Set("NotificationFilterDowntimeStart", 1 << NotificationDowntimeStart, true, true);
	ScriptVariable::Set("NotificationFilterDowntimeEnd", 1 << NotificationDowntimeEnd, true, true);
	ScriptVariable::Set("NotificationFilterDowntimeRemoved", 1 << NotificationDowntimeRemoved, true, true);
	ScriptVariable::Set("NotificationFilterCustom", 1 << NotificationCustom, true, true);
	ScriptVariable::Set("NotificationFilterAcknowledgement", 1 << NotificationAcknowledgement, true, true);
	ScriptVariable::Set("NotificationFilterProblem", 1 << NotificationProblem, true, true);
	ScriptVariable::Set("NotificationFilterRecovery", 1 << NotificationRecovery, true, true);
	ScriptVariable::Set("NotificationFilterFlappingStart", 1 << NotificationFlappingStart, true, true);
	ScriptVariable::Set("NotificationFilterFlappingEnd", 1 << NotificationFlappingEnd, true, true);
}

void Notification::Start(void)
{
	DynamicObject::Start();

	GetService()->AddNotification(GetSelf());
}

void Notification::Stop(void)
{
	DynamicObject::Stop();

	GetService()->RemoveNotification(GetSelf());
}

Service::Ptr Notification::GetService(void) const
{
	Host::Ptr host = Host::GetByName(GetHostRaw());

	if (GetServiceRaw().IsEmpty())
		return host->GetCheckService();
	else
		return host->GetServiceByShortName(GetServiceRaw());
}

NotificationCommand::Ptr Notification::GetNotificationCommand(void) const
{
	return NotificationCommand::GetByName(GetNotificationCommandRaw());
}

std::set<User::Ptr> Notification::GetUsers(void) const
{
	std::set<User::Ptr> result;

	Array::Ptr users = GetUsersRaw();

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

std::set<UserGroup::Ptr> Notification::GetUserGroups(void) const
{
	std::set<UserGroup::Ptr> result;

	Array::Ptr groups = GetUserGroupsRaw();

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

TimePeriod::Ptr Notification::GetNotificationPeriod(void) const
{
	return TimePeriod::GetByName(GetNotificationPeriodRaw());
}

double Notification::GetNextNotification(void) const
{
	return GetNextNotificationRaw();
}

/**
 * Sets the timestamp when the next periodical notification should be sent.
 * This does not affect notifications that are sent for state changes.
 */
void Notification::SetNextNotification(double time, const String& authority)
{
	SetNextNotificationRaw(time);

	OnNextNotificationChanged(GetSelf(), time, authority);
}

void Notification::UpdateNotificationNumber(void)
{
	SetNotificationNumber(GetNotificationNumber() + 1);
}

void Notification::ResetNotificationNumber(void)
{
	SetNotificationNumber(0);
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
		case NotificationFlappingStart:
			return "FLAPPINGSTART";
		case NotificationFlappingEnd:
			return "FLAPPINGEND";
		default:
			return "UNKNOWN_NOTIFICATION";
	}
}

void Notification::BeginExecuteNotification(NotificationType type, const CheckResult::Ptr& cr, bool force, const String& author, const String& text)
{
	ASSERT(!OwnsLock());

	if (!force) {
		TimePeriod::Ptr tp = GetNotificationPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogInformation, "icinga", "Not sending notifications for notification object '" + GetName() + "': not in timeperiod");
			return;
		}

		double now = Utility::GetTime();
		Dictionary::Ptr times = GetTimes();
		Service::Ptr service = GetService();

		if (type == NotificationProblem) {
			if (times && times->Contains("begin") && now < service->GetLastHardStateChange() + times->Get("begin")) {
				Log(LogInformation, "icinga", "Not sending notifications for notification object '" + GetName() + "': before escalation range");
				return;
			}

			if (times && times->Contains("end") && now > service->GetLastHardStateChange() + times->Get("end")) {
				Log(LogInformation, "icinga", "Not sending notifications for notification object '" + GetName() + "': after escalation range");
				return;
			}
		}

		unsigned long ftype = 1 << type;

		Log(LogDebug, "icinga", "FType=" + Convert::ToString(ftype) + ", TypeFilter=" + Convert::ToString(GetNotificationTypeFilter()));

		if (!(ftype & GetNotificationTypeFilter())) {
			Log(LogInformation, "icinga", "Not sending notifications for notification object '" + GetName() + "': type filter does not match");
			return;
		}

		unsigned long fstate = 1 << GetService()->GetState();

		if (!(fstate & GetNotificationStateFilter())) {
			Log(LogInformation, "icinga", "Not sending notifications for notification object '" + GetName() + "': state filter does not match");
			return;
		}
	}

	{
		ObjectLock olock(this);

		double now = Utility::GetTime();
		SetLastNotification(now);

		if (type == NotificationProblem)
			SetLastProblemNotification(now);
	}

	std::set<User::Ptr> allUsers;

	std::set<User::Ptr> users = GetUsers();
	std::copy(users.begin(), users.end(), std::inserter(allUsers, allUsers.begin()));

	BOOST_FOREACH(const UserGroup::Ptr& ug, GetUserGroups()) {
		std::set<User::Ptr> members = ug->GetMembers();
		std::copy(members.begin(), members.end(), std::inserter(allUsers, allUsers.begin()));
	}

	unsigned long notified_users = 0;
	BOOST_FOREACH(const User::Ptr& user, allUsers) {
		Log(LogDebug, "icinga", "Sending notification for user '" + user->GetName() + "'");
		Utility::QueueAsyncCallback(boost::bind(&Notification::ExecuteNotificationHelper, this, type, user, cr, force, author, text));
		notified_users++;
	}

	Service::OnNotificationSentToAllUsers(GetService(), allUsers, type, cr, author, text);
}

void Notification::ExecuteNotificationHelper(NotificationType type, const User::Ptr& user, const CheckResult::Ptr& cr, bool force, const String& author, const String& text)
{
	ASSERT(!OwnsLock());

	if (!force) {
		TimePeriod::Ptr tp = user->GetNotificationPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogInformation, "icinga", "Not sending notifications for notification object '" +
			    GetName() + " and user '" + user->GetName() + "': user not in timeperiod");
			return;
		}

		unsigned long ftype = 1 << type;

		if (!(ftype & user->GetNotificationTypeFilter())) {
			Log(LogInformation, "icinga", "Not sending notifications for notification object '" +
			    GetName() + " and user '" + user->GetName() + "': type filter does not match");
			return;
		}

		unsigned long fstate = 1 << GetService()->GetState();

		if (!(fstate & user->GetNotificationStateFilter())) {
			Log(LogInformation, "icinga", "Not sending notifications for notification object '" +
			    GetName() + " and user '" + user->GetName() + "': state filter does not match");
			return;
		}
	}

	try {
		NotificationCommand::Ptr command = GetNotificationCommand();

		if (!command) {
			Log(LogDebug, "icinga", "No notification_command found for notification '" + GetName() + "'. Skipping execution.");
			return;
		}

		command->Execute(GetSelf(), user, cr, type, author, text);

		{
			ObjectLock olock(this);
			UpdateNotificationNumber();
			SetLastNotification(Utility::GetTime());
		}

		Service::OnNotificationSentToUser(GetService(), user, type, cr, author, text, command->GetName());

		Log(LogInformation, "icinga", "Completed sending notification for service '" + GetService()->GetName() + "'");
	} catch (const std::exception& ex) {
		std::ostringstream msgbuf;
		msgbuf << "Exception occured during notification for service '"
		       << GetService()->GetName() << "': " << DiagnosticInformation(ex);
		Log(LogWarning, "icinga", msgbuf.str());
	}
}

bool Notification::ResolveMacro(const String& macro, const CheckResult::Ptr&, String *result) const
{
	if (macro.SubStr(0, 13) == "_NOTIFICATION") {
		Dictionary::Ptr custom = GetCustom();
		*result = custom ? custom->Get(macro.SubStr(13)) : "";
		return true;
	}

	Dictionary::Ptr macros = GetMacros();

	if (macros && macros->Contains(macro)) {
		*result = macros->Get(macro);
		return true;
	}

	return false;
}
