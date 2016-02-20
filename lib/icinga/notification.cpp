/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include "icinga/notification.tcpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/service.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/initialize.hpp"
#include "base/scriptglobal.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

using namespace icinga;

REGISTER_TYPE(Notification);
INITIALIZE_ONCE(&Notification::StaticInitialize);

boost::signals2::signal<void (const Notification::Ptr&, const MessageOrigin::Ptr&)> Notification::OnNextNotificationChanged;

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

Dictionary::Ptr NotificationNameComposer::ParseName(const String& name) const
{
	std::vector<String> tokens;
	boost::algorithm::split(tokens, name, boost::is_any_of("!"));

	if (tokens.size() < 2)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid Notification name."));

	Dictionary::Ptr result = new Dictionary();
	result->Set("host_name", tokens[0]);

	if (tokens.size() > 2) {
		result->Set("service_name", tokens[1]);
		result->Set("name", tokens[2]);
	} else {
		result->Set("name", tokens[1]);
	}

	return result;
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
	ObjectImpl<Notification>::OnConfigLoaded();

	SetTypeFilter(FilterArrayToInt(GetTypes(), ~0));
	SetStateFilter(FilterArrayToInt(GetStates(), ~0));
}

void Notification::OnAllConfigLoaded(void)
{
	ObjectImpl<Notification>::OnAllConfigLoaded();

	Host::Ptr host = Host::GetByName(GetHostName());

	if (GetServiceName().IsEmpty())
		m_Checkable = host;
	else
		m_Checkable = host->GetServiceByShortName(GetServiceName());

	if (!m_Checkable)
		BOOST_THROW_EXCEPTION(ScriptError("Notification object refers to a host/service which doesn't exist.", GetDebugInfo()));

	GetCheckable()->RegisterNotification(this);
}

void Notification::Start(bool runtimeCreated)
{
	ObjectImpl<Notification>::Start(runtimeCreated);

	Checkable::Ptr obj = GetCheckable();

	if (obj)
		obj->RegisterNotification(this);
}

void Notification::Stop(bool runtimeRemoved)
{
	ObjectImpl<Notification>::Stop(runtimeRemoved);

	Checkable::Ptr obj = GetCheckable();

	if (obj)
		obj->UnregisterNotification(this);
}

Checkable::Ptr Notification::GetCheckable(void) const
{
	return static_pointer_cast<Checkable>(m_Checkable);
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

void Notification::UpdateNotificationNumber(void)
{
	SetNotificationNumber(GetNotificationNumber() + 1);
}

void Notification::ResetNotificationNumber(void)
{
	SetNotificationNumber(0);
}

/* the upper case string used in all interfaces */
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
	Log(LogNotice, "Notification")
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
		    << "Type '" << NotificationTypeToStringInternal(type)
		    << "', TypeFilter " << NotificationFilterToString(GetTypeFilter())
		    << " (FType=" << ftype << ", TypeFilter=" << GetTypeFilter() << ")";

		if (!(ftype & GetTypeFilter())) {
			Log(LogNotice, "Notification")
			    << "Not sending notifications for notification object '" << GetName() << "': type '"
			    << NotificationTypeToStringInternal(type) << "' does not match type filter '"
			    << NotificationFilterToString(GetTypeFilter()) << ".";
			return;
		}

		/* ensure that recovery notifications are always sent, no state filter checks necessary */
		if (type != NotificationRecovery) {
			Host::Ptr host;
			Service::Ptr service;
			tie(host, service) = GetHostService(checkable);

			unsigned long fstate;
			String stateStr;

			if (service) {
				fstate = ServiceStateToFilter(service->GetState());
				stateStr = NotificationServiceStateToString(service->GetState());
			} else {
				fstate = HostStateToFilter(host->GetState());
				stateStr = NotificationHostStateToString(host->GetState());
			}

			Log(LogDebug, "Notification")
			    << "State '" << stateStr << "', StateFilter " << NotificationFilterToString(GetStateFilter())
			    << " (FState=" << fstate << ", StateFilter=" << GetStateFilter() << ")";

			if (!(fstate & GetStateFilter())) {
				Log(LogNotice, "Notification")
				    << "Not sending notifications for notification object '" << GetName() << "': state '" << stateStr
				    << "' does not match state filter " << NotificationFilterToString(GetStateFilter()) << ".";
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
	if (!force) {
		TimePeriod::Ptr tp = user->GetPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogNotice, "Notification")
			    << "Not sending notifications for notification object '"
			    << GetName() << " and user '" << user->GetName() << "': user not in timeperiod";
			return false;
		}

		unsigned long ftype = 1 << type;

		Log(LogDebug, "Notification")
		    << "User notification, Type '" << NotificationTypeToStringInternal(type)
		    << "', TypeFilter " << NotificationFilterToString(user->GetTypeFilter())
		    << " (FType=" << ftype << ", TypeFilter=" << GetTypeFilter() << ")";


		if (!(ftype & user->GetTypeFilter())) {
			Log(LogNotice, "Notification")
			    << "Not sending notifications for notification object '"
			    << GetName() << " and user '" << user->GetName() << "': type '"
			    << NotificationTypeToStringInternal(type) << "' does not match type filter "
			    << NotificationFilterToString(user->GetTypeFilter()) << ".";
			return false;
		}

		/* check state filters it this is not a recovery notification */
		if (type != NotificationRecovery) {
			Checkable::Ptr checkable = GetCheckable();
			Host::Ptr host;
			Service::Ptr service;
			tie(host, service) = GetHostService(checkable);

			unsigned long fstate;
			String stateStr;

			if (service) {
				fstate = ServiceStateToFilter(service->GetState());
				stateStr = NotificationServiceStateToString(service->GetState());
			} else {
				fstate = HostStateToFilter(host->GetState());
				stateStr = NotificationHostStateToString(host->GetState());
			}

			Log(LogDebug, "Notification")
			    << "User notification, State '" << stateStr << "', StateFilter "
			    << NotificationFilterToString(user->GetStateFilter())
			    << " (FState=" << fstate << ", StateFilter=" << user->GetStateFilter() << ")";

			if (!(fstate & user->GetStateFilter())) {
				Log(LogNotice, "Notification")
				    << "Not sending notifications for notification object '"
				    << GetName() << " and user '" << user->GetName() << "': state '" << stateStr
				    << "' does not match state filter " << NotificationFilterToString(user->GetStateFilter()) << ".";
				return false;
			}
		}
	}

	return true;
}

void Notification::ExecuteNotificationHelper(NotificationType type, const User::Ptr& user, const CheckResult::Ptr& cr, bool force, const String& author, const String& text)
{
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

std::vector<String> icinga::FilterIntToArray(int iFilter)
{
	std::vector<String> filter;

	if (iFilter & StateFilterOK)
		filter.push_back("OK");
	if (iFilter & StateFilterWarning)
		filter.push_back("Warning");
	if (iFilter & StateFilterUnknown)
		filter.push_back("Unknown");
	if (iFilter & StateFilterUp)
		filter.push_back("Up");
	if (iFilter & StateFilterDown)
		filter.push_back("Down");
	if (iFilter & NotificationDowntimeStart)
		filter.push_back("DowntimeStart");
	if (iFilter & NotificationDowntimeEnd)
		filter.push_back("DowntimeEnd");
	if (iFilter & NotificationDowntimeRemoved)
		filter.push_back("DowntimeRemoved");
	if (iFilter & NotificationCustom)
		filter.push_back("Custom");
	if (iFilter & NotificationAcknowledgement)
		filter.push_back("Acknowledgement");
	if (iFilter & NotificationProblem)
		filter.push_back("Problem");
	if (iFilter & NotificationRecovery)
		filter.push_back("Recovery");
	if (iFilter & NotificationFlappingStart)
		filter.push_back("FlappingStart");
	if (iFilter & NotificationFlappingEnd)
		filter.push_back("FlappingEnd");

	return filter;
}

String Notification::NotificationFilterToString(int filter)
{
	return Utility::NaturalJoin(FilterIntToArray(filter));
}

/* internal for logging */
String Notification::NotificationTypeToStringInternal(NotificationType type)
{
	switch (type) {
		case NotificationDowntimeStart:
			return "DowntimeStart";
		case NotificationDowntimeEnd:
			return "DowntimeEnd";
		case NotificationDowntimeRemoved:
			return "DowntimeRemoved";
		case NotificationCustom:
			return "Custom";
		case NotificationAcknowledgement:
			return "Acknowledgement";
		case NotificationProblem:
			return "Problem";
		case NotificationRecovery:
			return "Recovery";
		case NotificationFlappingStart:
			return "FlappingStart";
		case NotificationFlappingEnd:
			return "FlappingEnd";
		default:
			return Empty;
	}
}

String Notification::NotificationServiceStateToString(ServiceState state)
{
	switch (state) {
		case ServiceOK:
			return "OK";
		case ServiceWarning:
			return "Warning";
		case ServiceCritical:
			return "Critical";
		case ServiceUnknown:
			return "Unknown";
		default:
			VERIFY(!"Invalid state type.");
	}
}

String Notification::NotificationHostStateToString(HostState state)
{
	switch (state) {
		case HostUp:
			return "Up";
		case HostDown:
			return "Down";
		default:
			VERIFY(!"Invalid state type.");
	}
}

void Notification::Validate(int types, const ValidationUtils& utils)
{
	ObjectImpl<Notification>::Validate(types, utils);

	if (!(types & FAConfig))
		return;

	Array::Ptr users = GetUsersRaw();
	Array::Ptr groups = GetUserGroupsRaw();

	if ((!users || users->GetLength() == 0) && (!groups || groups->GetLength() == 0))
		BOOST_THROW_EXCEPTION(ValidationError(this, std::vector<String>(), "Validation failed: No users/user_groups specified."));
}

void Notification::ValidateStates(const Array::Ptr& value, const ValidationUtils& utils)
{
	ObjectImpl<Notification>::ValidateStates(value, utils);

	int sfilter = FilterArrayToInt(value, 0);

	if (GetServiceName().IsEmpty() && (sfilter & ~(StateFilterUp | StateFilterDown)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("states"), "State filter is invalid."));

	if (!GetServiceName().IsEmpty() && (sfilter & ~(StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("types"), "State filter is invalid."));
}

void Notification::ValidateTypes(const Array::Ptr& value, const ValidationUtils& utils)
{
	ObjectImpl<Notification>::ValidateTypes(value, utils);

	int tfilter = FilterArrayToInt(value, 0);

	if ((tfilter & ~(1 << NotificationDowntimeStart | 1 << NotificationDowntimeEnd | 1 << NotificationDowntimeRemoved |
	    1 << NotificationCustom | 1 << NotificationAcknowledgement | 1 << NotificationProblem | 1 << NotificationRecovery |
	    1 << NotificationFlappingStart | 1 << NotificationFlappingEnd)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("types"), "Type filter is invalid."));
}

Endpoint::Ptr Notification::GetCommandEndpoint(void) const
{
	return Endpoint::GetByName(GetCommandEndpointRaw());
}

