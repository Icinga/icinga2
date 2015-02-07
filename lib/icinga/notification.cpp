/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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

#include "icinga/notification.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/service.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/initialize.hpp"
#include "base/scriptglobal.hpp"
#include "base/function.hpp"
#include <boost/foreach.hpp>

using namespace icinga;

REGISTER_TYPE(Notification);
REGISTER_SCRIPTFUNCTION(ValidateNotificationFilters, &Notification::ValidateFilters);
REGISTER_SCRIPTFUNCTION(ValidateNotificationUsers, &Notification::ValidateUsers);
INITIALIZE_ONCE(&Notification::StaticInitialize);

boost::signals2::signal<void (const Notification::Ptr&, double, const MessageOrigin&)> Notification::OnNextNotificationChanged;

String NotificationNameComposer::MakeName(const String& shortName, const Object::Ptr& context) const
{
	Notification::Ptr notification = dynamic_pointer_cast<Notification>(context);

	if (!notification)
		return "";

	String name = notification->GetHostName();

	if (!notification->GetServiceName().IsEmpty())
		name += "!" + notification->GetServiceName();

	name += "!" + shortName;

	return name;
}

void Notification::StaticInitialize(void)
{
	ScriptGlobal::Set("OK", StateFilterOK);
	ScriptGlobal::Set("Warning", StateFilterWarning);
	ScriptGlobal::Set("Critical", StateFilterCritical);
	ScriptGlobal::Set("Unknown", StateFilterUnknown);
	ScriptGlobal::Set("Up", StateFilterUp);
	ScriptGlobal::Set("Down", StateFilterDown);

	ScriptGlobal::Set("DowntimeStart", 1 << NotificationDowntimeStart);
	ScriptGlobal::Set("DowntimeEnd", 1 << NotificationDowntimeEnd);
	ScriptGlobal::Set("DowntimeRemoved", 1 << NotificationDowntimeRemoved);
	ScriptGlobal::Set("Custom", 1 << NotificationCustom);
	ScriptGlobal::Set("Acknowledgement", 1 << NotificationAcknowledgement);
	ScriptGlobal::Set("Problem", 1 << NotificationProblem);
	ScriptGlobal::Set("Recovery", 1 << NotificationRecovery);
	ScriptGlobal::Set("FlappingStart", 1 << NotificationFlappingStart);
	ScriptGlobal::Set("FlappingEnd", 1 << NotificationFlappingEnd);
}

void Notification::OnConfigLoaded(void)
{
	SetTypeFilter(FilterArrayToInt(GetTypes(), ~0));
	SetStateFilter(FilterArrayToInt(GetStates(), ~0));
}

void Notification::OnAllConfigLoaded(void)
{
	Checkable::Ptr obj = GetCheckable();

	if (!obj)
		BOOST_THROW_EXCEPTION(ScriptError("Notification object refers to a host/service which doesn't exist.", GetDebugInfo()));

	obj->AddNotification(this);
}

void Notification::Start(void)
{
	DynamicObject::Start();

	Checkable::Ptr obj = GetCheckable();

	if (obj)
		obj->AddNotification(this);
}

void Notification::Stop(void)
{
	DynamicObject::Stop();

	Checkable::Ptr obj = GetCheckable();

	if (obj)
		obj->RemoveNotification(this);
}

Checkable::Ptr Notification::GetCheckable(void) const
{
	Host::Ptr host = Host::GetByName(GetHostName());

	if (GetServiceName().IsEmpty())
		return host;
	else
		return host->GetServiceByShortName(GetServiceName());
}

NotificationCommand::Ptr Notification::GetCommand(void) const
{
	return NotificationCommand::GetByName(GetCommandRaw());
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

TimePeriod::Ptr Notification::GetPeriod(void) const
{
	return TimePeriod::GetByName(GetPeriodRaw());
}

double Notification::GetNextNotification(void) const
{
	return GetNextNotificationRaw();
}

/**
 * Sets the timestamp when the next periodical notification should be sent.
 * This does not affect notifications that are sent for state changes.
 */
void Notification::SetNextNotification(double time, const MessageOrigin& origin)
{
	SetNextNotificationRaw(time);

	OnNextNotificationChanged(this, time, origin);
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

	Log(LogInformation, "Notification")
	    << "Attempting to send notifications for notification object '" << GetName() << "'.";

	Checkable::Ptr checkable = GetCheckable();

	if (!force) {
		TimePeriod::Ptr tp = GetPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogNotice, "Notification")
			    << "Not sending notifications for notification object '" << GetName() << "': not in timeperiod";
			return;
		}

		double now = Utility::GetTime();
		Dictionary::Ptr times = GetTimes();

		if (type == NotificationProblem) {
			if (times && times->Contains("begin") && now < checkable->GetLastHardStateChange() + times->Get("begin")) {
				Log(LogNotice, "Notification")
				    << "Not sending notifications for notification object '" << GetName() << "': before escalation range";

				/* we need to adjust the next notification time
 				 * to now + begin delaying the first notification
 				 */
				double nextProposedNotification = now + times->Get("begin") + 1.0;
				if (GetNextNotification() > nextProposedNotification)
					SetNextNotification(nextProposedNotification);

				return;
			}

			if (times && times->Contains("end") && now > checkable->GetLastHardStateChange() + times->Get("end")) {
				Log(LogNotice, "Notification")
				    << "Not sending notifications for notification object '" << GetName() << "': after escalation range";
				return;
			}
		}

		unsigned long ftype = 1 << type;

		Log(LogDebug, "Notification")
		    << "FType=" << ftype << ", TypeFilter=" << GetTypeFilter();

		if (!(ftype & GetTypeFilter())) {
			Log(LogNotice, "Notification")
			    << "Not sending notifications for notification object '" << GetName() << "': type filter does not match '"
			    << NotificationTypeToString(type) << "'";
			return;
		}

		/* ensure that recovery notifications are always sent, no state filter checks necessary */
		if (type != NotificationRecovery) {
			Host::Ptr host;
			Service::Ptr service;
			tie(host, service) = GetHostService(checkable);

			unsigned long fstate;

			if (service)
				fstate = ServiceStateToFilter(service->GetState());
			else
				fstate = HostStateToFilter(host->GetState());

			if (!(fstate & GetStateFilter())) {
				Log(LogNotice, "Notification")
				    << "Not sending notifications for notification object '" << GetName() << "': state filter does not match";
				return;
			}
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

	Service::OnNotificationSendStart(this, checkable, allUsers, type, cr, author, text);

	std::set<User::Ptr> allNotifiedUsers;
	Array::Ptr notifiedUsers = GetNotifiedUsers();

	BOOST_FOREACH(const User::Ptr& user, allUsers) {
		String userName = user->GetName();

		if (!user->GetEnableNotifications()) {
			Log(LogNotice, "Notification")
			    << "Disabled notifications for user '" << userName << "'. Not sending notification.";
			continue;
		}

		if (!CheckNotificationUserFilters(type, user, force)) {
			Log(LogNotice, "Notification")
			    << "Notification filters for user '" << userName << "' not matched. Not sending notification.";
			continue;
		}

		/* on recovery, check if user was notified before */
		if (type == NotificationRecovery) {
			if (!notifiedUsers->Contains(userName)) {
				Log(LogNotice, "Notification")
				    << "We did not notify user '" << userName << "' before. Not sending recovery notification.";
				continue;
			}
		}

		Log(LogInformation, "Notification")
		    << "Sending notification '" << GetName() << "' for user '" << userName << "'";

		Utility::QueueAsyncCallback(boost::bind(&Notification::ExecuteNotificationHelper, this, type, user, cr, force, author, text));

		/* collect all notified users */
		allNotifiedUsers.insert(user);

		/* store all notified users for later recovery checks */
		if (!notifiedUsers->Contains(userName))
			notifiedUsers->Add(userName);
	}

	/* if this was a recovery notification, reset all notified users */
	if (type == NotificationRecovery)
		notifiedUsers->Clear();

	/* used in db_ido for notification history */
	Service::OnNotificationSentToAllUsers(this, checkable, allNotifiedUsers, type, cr, author, text);
}

bool Notification::CheckNotificationUserFilters(NotificationType type, const User::Ptr& user, bool force)
{
	ASSERT(!OwnsLock());

	if (!force) {
		TimePeriod::Ptr tp = user->GetPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogNotice, "Notification")
			    << "Not sending notifications for notification object '"
			    << GetName() << " and user '" << user->GetName() << "': user not in timeperiod";
			return false;
		}

		unsigned long ftype = 1 << type;

		if (!(ftype & user->GetTypeFilter())) {
			Log(LogNotice, "Notification")
			    << "Not sending notifications for notification object '"
			    << GetName() << " and user '" << user->GetName() << "': type filter does not match";
			return false;
		}

		/* check state filters it this is not a recovery notification */
		if (type != NotificationRecovery) {
			Checkable::Ptr checkable = GetCheckable();
			Host::Ptr host;
			Service::Ptr service;
			tie(host, service) = GetHostService(checkable);

			unsigned long fstate;

			if (service)
					fstate = ServiceStateToFilter(service->GetState());
			else
					fstate = HostStateToFilter(host->GetState());

			if (!(fstate & user->GetStateFilter())) {
				Log(LogNotice, "Notification")
				    << "Not sending notifications for notification object '"
				    << GetName() << " and user '" << user->GetName() << "': state filter does not match";
				return false;
			}
		}
	}

	return true;
}

void Notification::ExecuteNotificationHelper(NotificationType type, const User::Ptr& user, const CheckResult::Ptr& cr, bool force, const String& author, const String& text)
{
	ASSERT(!OwnsLock());

	try {
		NotificationCommand::Ptr command = GetCommand();

		if (!command) {
			Log(LogDebug, "Notification")
			    << "No notification_command found for notification '" << GetName() << "'. Skipping execution.";
			return;
		}

		command->Execute(this, user, cr, type, author, text);

		{
			ObjectLock olock(this);
			UpdateNotificationNumber();
			SetLastNotification(Utility::GetTime());
		}

		/* required by compatlogger */
		Service::OnNotificationSentToUser(this, GetCheckable(), user, type, cr, author, text, command->GetName());

		Log(LogInformation, "Notification")
		    << "Completed sending notification '" << GetName() << "' for checkable '" << GetCheckable()->GetName() << "'";
	} catch (const std::exception& ex) {
		Log(LogWarning, "Notification")
		    << "Exception occured during notification for checkable '"
		    << GetCheckable()->GetName() << "': " << DiagnosticInformation(ex);
	}
}

int icinga::ServiceStateToFilter(ServiceState state)
{
	switch (state) {
		case ServiceOK:
			return StateFilterOK;
		case ServiceWarning:
			return StateFilterWarning;
		case ServiceCritical:
			return StateFilterCritical;
		case ServiceUnknown:
			return StateFilterUnknown;
		default:
			VERIFY(!"Invalid state type.");
	}
}

int icinga::HostStateToFilter(HostState state)
{
	switch (state) {
		case HostUp:
			return StateFilterUp;
		case HostDown:
			return StateFilterDown;
		default:
			VERIFY(!"Invalid state type.");
	}
}

int icinga::FilterArrayToInt(const Array::Ptr& typeFilters, int defaultValue)
{
	Value resultTypeFilter;

	if (!typeFilters)
		return defaultValue;

	resultTypeFilter = 0;

	ObjectLock olock(typeFilters);
	BOOST_FOREACH(const Value& typeFilter, typeFilters) {
		resultTypeFilter = resultTypeFilter | typeFilter;
	}

	return resultTypeFilter;
}

void Notification::ValidateUsers(const String& location, const Notification::Ptr& object)
{
	std::set<User::Ptr> allUsers;

	std::set<User::Ptr> users = object->GetUsers();
	std::copy(users.begin(), users.end(), std::inserter(allUsers, allUsers.begin()));

	BOOST_FOREACH(const UserGroup::Ptr& ug, object->GetUserGroups()) {
		std::set<User::Ptr> members = ug->GetMembers();
		std::copy(members.begin(), members.end(), std::inserter(allUsers, allUsers.begin()));
	}

	if (allUsers.empty()) {
		BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
		    location + ": No users/user_groups specified.", object->GetDebugInfo()));
	}
}

void Notification::ValidateFilters(const String& location, const Notification::Ptr& object)
{
	int sfilter = FilterArrayToInt(object->GetStates(), 0);

	if (object->GetServiceName().IsEmpty() && (sfilter & ~(StateFilterUp | StateFilterDown)) != 0) {
		BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
		    location + ": State filter is invalid.", object->GetDebugInfo()));
	}

	if (!object->GetServiceName().IsEmpty() && (sfilter & ~(StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown)) != 0) {
		BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
		    location + ": State filter is invalid.", object->GetDebugInfo()));
	}

	int tfilter = FilterArrayToInt(object->GetTypes(), 0);

	if ((tfilter & ~(1 << NotificationDowntimeStart | 1 << NotificationDowntimeEnd | 1 << NotificationDowntimeRemoved |
	    1 << NotificationCustom | 1 << NotificationAcknowledgement | 1 << NotificationProblem | 1 << NotificationRecovery |
	    1 << NotificationFlappingStart | 1 << NotificationFlappingEnd)) != 0) {
		BOOST_THROW_EXCEPTION(ScriptError("Validation failed for " +
		    location + ": Type filter is invalid.", object->GetDebugInfo()));
	}
}

Endpoint::Ptr Notification::GetCommandEndpoint(void) const
{
	return Endpoint::GetByName(GetCommandEndpointRaw());
}

