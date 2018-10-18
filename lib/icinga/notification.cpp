/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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
#include "icinga/notification-ti.cpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/service.hpp"
#include "remote/apilistener.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/exception.hpp"
#include "base/initialize.hpp"
#include "base/scriptglobal.hpp"

using namespace icinga;

REGISTER_TYPE(Notification);
INITIALIZE_ONCE(&Notification::StaticInitialize);

std::map<String, int> Notification::m_StateFilterMap;
std::map<String, int> Notification::m_TypeFilterMap;

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
	std::vector<String> tokens = name.Split("!");

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

void Notification::StaticInitialize()
{
	ScriptGlobal::Set("Icinga.OK", "OK", true);
	ScriptGlobal::Set("Icinga.Warning", "Warning", true);
	ScriptGlobal::Set("Icinga.Critical", "Critical", true);
	ScriptGlobal::Set("Icinga.Unknown", "Unknown", true);
	ScriptGlobal::Set("Icinga.Up", "Up", true);
	ScriptGlobal::Set("Icinga.Down", "Down", true);

	ScriptGlobal::Set("Icinga.DowntimeStart", "DowntimeStart", true);
	ScriptGlobal::Set("Icinga.DowntimeEnd", "DowntimeEnd", true);
	ScriptGlobal::Set("Icinga.DowntimeRemoved", "DowntimeRemoved", true);
	ScriptGlobal::Set("Icinga.Custom", "Custom", true);
	ScriptGlobal::Set("Icinga.Acknowledgement", "Acknowledgement", true);
	ScriptGlobal::Set("Icinga.Problem", "Problem", true);
	ScriptGlobal::Set("Icinga.Recovery", "Recovery", true);
	ScriptGlobal::Set("Icinga.FlappingStart", "FlappingStart", true);
	ScriptGlobal::Set("Icinga.FlappingEnd", "FlappingEnd", true);

	m_StateFilterMap["OK"] = StateFilterOK;
	m_StateFilterMap["Warning"] = StateFilterWarning;
	m_StateFilterMap["Critical"] = StateFilterCritical;
	m_StateFilterMap["Unknown"] = StateFilterUnknown;
	m_StateFilterMap["Up"] = StateFilterUp;
	m_StateFilterMap["Down"] = StateFilterDown;

	m_TypeFilterMap["DowntimeStart"] = NotificationDowntimeStart;
	m_TypeFilterMap["DowntimeEnd"] = NotificationDowntimeEnd;
	m_TypeFilterMap["DowntimeRemoved"] = NotificationDowntimeRemoved;
	m_TypeFilterMap["Custom"] = NotificationCustom;
	m_TypeFilterMap["Acknowledgement"] = NotificationAcknowledgement;
	m_TypeFilterMap["Problem"] = NotificationProblem;
	m_TypeFilterMap["Recovery"] = NotificationRecovery;
	m_TypeFilterMap["FlappingStart"] = NotificationFlappingStart;
	m_TypeFilterMap["FlappingEnd"] = NotificationFlappingEnd;
}

void Notification::OnConfigLoaded()
{
	ObjectImpl<Notification>::OnConfigLoaded();

	SetTypeFilter(FilterArrayToInt(GetTypes(), GetTypeFilterMap(), ~0));
	SetStateFilter(FilterArrayToInt(GetStates(), GetStateFilterMap(), ~0));
}

void Notification::OnAllConfigLoaded()
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
	Checkable::Ptr obj = GetCheckable();

	if (obj)
		obj->RegisterNotification(this);

	if (ApiListener::IsHACluster() && GetNextNotification() < Utility::GetTime() + 60)
		SetNextNotification(Utility::GetTime() + 60, true);

	ObjectImpl<Notification>::Start(runtimeCreated);
}

void Notification::Stop(bool runtimeRemoved)
{
	ObjectImpl<Notification>::Stop(runtimeRemoved);

	Checkable::Ptr obj = GetCheckable();

	if (obj)
		obj->UnregisterNotification(this);
}

Checkable::Ptr Notification::GetCheckable() const
{
	return static_pointer_cast<Checkable>(m_Checkable);
}

NotificationCommand::Ptr Notification::GetCommand() const
{
	return NotificationCommand::GetByName(GetCommandRaw());
}

std::set<User::Ptr> Notification::GetUsers() const
{
	std::set<User::Ptr> result;

	Array::Ptr users = GetUsersRaw();

	if (users) {
		ObjectLock olock(users);

		for (const String& name : users) {
			User::Ptr user = User::GetByName(name);

			if (!user)
				continue;

			result.insert(user);
		}
	}

	return result;
}

std::set<UserGroup::Ptr> Notification::GetUserGroups() const
{
	std::set<UserGroup::Ptr> result;

	Array::Ptr groups = GetUserGroupsRaw();

	if (groups) {
		ObjectLock olock(groups);

		for (const String& name : groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (!ug)
				continue;

			result.insert(ug);
		}
	}

	return result;
}

TimePeriod::Ptr Notification::GetPeriod() const
{
	return TimePeriod::GetByName(GetPeriodRaw());
}

void Notification::UpdateNotificationNumber()
{
	SetNotificationNumber(GetNotificationNumber() + 1);
}

void Notification::ResetNotificationNumber()
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

void Notification::BeginExecuteNotification(NotificationType type, const CheckResult::Ptr& cr, bool force, bool reminder, const String& author, const String& text)
{
	Log(LogNotice, "Notification")
		<< "Attempting to send " << (reminder ? "reminder " : " ") << "notifications for notification object '" << GetName() << "'.";

	Checkable::Ptr checkable = GetCheckable();

	if (!force) {
		TimePeriod::Ptr tp = GetPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogNotice, "Notification")
				<< "Not sending " << (reminder ? "reminder " : " ") << "notifications for notification object '" << GetName()
				<< "': not in timeperiod '" << tp->GetName() << "'";
			return;
		}

		double now = Utility::GetTime();
		Dictionary::Ptr times = GetTimes();

		if (times && type == NotificationProblem) {
			Value timesBegin = times->Get("begin");
			Value timesEnd = times->Get("end");

			if (timesBegin != Empty && timesBegin >= 0 && now < checkable->GetLastHardStateChange() + timesBegin) {
				Log(LogNotice, "Notification")
					<< "Not sending " << (reminder ? "reminder " : " ") << "notifications for notification object '" << GetName()
					<< "': before specified begin time (" << Utility::FormatDuration(timesBegin) << ")";

				/* we need to adjust the next notification time
				 * to now + begin delaying the first notification
				 */
				double nextProposedNotification = now + timesBegin + 1.0;
				if (GetNextNotification() > nextProposedNotification)
					SetNextNotification(nextProposedNotification);

				return;
			}

			if (timesEnd != Empty && timesEnd >= 0 && now > checkable->GetLastHardStateChange() + timesEnd) {
				Log(LogNotice, "Notification")
					<< "Not sending " << (reminder ? "reminder " : " ") << "notifications for notification object '" << GetName()
					<< "': after specified end time (" << Utility::FormatDuration(timesEnd) << ")";
				return;
			}
		}

		unsigned long ftype = type;

		Log(LogDebug, "Notification")
			<< "Type '" << NotificationTypeToStringInternal(type)
			<< "', TypeFilter: " << NotificationFilterToString(GetTypeFilter(), GetTypeFilterMap())
			<< " (FType=" << ftype << ", TypeFilter=" << GetTypeFilter() << ")";

		if (!(ftype & GetTypeFilter())) {
			Log(LogNotice, "Notification")
				<< "Not sending " << (reminder ? "reminder " : " ") << "notifications for notification object '" << GetName() << "': type '"
				<< NotificationTypeToStringInternal(type) << "' does not match type filter: "
				<< NotificationFilterToString(GetTypeFilter(), GetTypeFilterMap()) << ".";
			return;
		}

		/* Check state filters for problem notifications. Recovery notifications will be filtered away later. */
		if (type == NotificationProblem) {
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
				<< "State '" << stateStr << "', StateFilter: " << NotificationFilterToString(GetStateFilter(), GetStateFilterMap())
				<< " (FState=" << fstate << ", StateFilter=" << GetStateFilter() << ")";

			if (!(fstate & GetStateFilter())) {
				Log(LogNotice, "Notification")
					<< "Not sending " << (reminder ? "reminder " : " ") << "notifications for notification object '" << GetName() << "': state '" << stateStr
					<< "' does not match state filter: " << NotificationFilterToString(GetStateFilter(), GetStateFilterMap()) << ".";
				return;
			}
		}
	} else {
		Log(LogNotice, "Notification")
			<< "Not checking " << (reminder ? "reminder " : " ") << "notification filters for notification object '" << GetName() << "': Notification was forced.";
	}

	{
		ObjectLock olock(this);

		UpdateNotificationNumber();
		double now = Utility::GetTime();
		SetLastNotification(now);

		if (type == NotificationProblem && GetInterval() <= 0)
			SetNoMoreNotifications(true);
		else
			SetNoMoreNotifications(false);

		if (type == NotificationProblem && GetInterval() > 0)
			SetNextNotification(now + GetInterval());

		if (type == NotificationProblem)
			SetLastProblemNotification(now);
	}

	std::set<User::Ptr> allUsers;

	std::set<User::Ptr> users = GetUsers();
	std::copy(users.begin(), users.end(), std::inserter(allUsers, allUsers.begin()));

	for (const UserGroup::Ptr& ug : GetUserGroups()) {
		std::set<User::Ptr> members = ug->GetMembers();
		std::copy(members.begin(), members.end(), std::inserter(allUsers, allUsers.begin()));
	}

	std::set<User::Ptr> allNotifiedUsers;
	Array::Ptr notifiedProblemUsers = GetNotifiedProblemUsers();

	for (const User::Ptr& user : allUsers) {
		String userName = user->GetName();

		if (!user->GetEnableNotifications()) {
			Log(LogNotice, "Notification")
				<< "Disabled notifications for user '" << userName << "'. Not sending notification.";
			continue;
		}

		if (!CheckNotificationUserFilters(type, user, force, reminder)) {
			Log(LogNotice, "Notification")
				<< "Notification filters for user '" << userName << "' not matched. Not sending notification.";
			continue;
		}

		/* on recovery, check if user was notified before */
		if (type == NotificationRecovery) {
			if (!notifiedProblemUsers->Contains(userName) && (NotificationProblem & user->GetTypeFilter())) {
				Log(LogNotice, "Notification")
					<< "We did not notify user '" << userName
					<< "' (Problem types enabled) for a problem before. Not sending recovery notification.";
				continue;
			}
		}

		/* on acknowledgement, check if user was notified before */
		if (type == NotificationAcknowledgement) {
			if (!notifiedProblemUsers->Contains(userName) && (NotificationProblem & user->GetTypeFilter())) {
				Log(LogNotice, "Notification")
					<< "We did not notify user '" << userName
					<< "' (Problem types enabled) for a problem before. Not sending acknowledgement notification.";
				continue;
			}
		}

		Log(LogInformation, "Notification")
			<< "Sending " << (reminder ? "reminder " : "") << "'" << NotificationTypeToStringInternal(type) << "' notification '"
			<< GetName() << "' for user '" << userName << "'";

		Utility::QueueAsyncCallback(std::bind(&Notification::ExecuteNotificationHelper, this, type, user, cr, force, author, text));

		/* collect all notified users */
		allNotifiedUsers.insert(user);

		/* store all notified users for later recovery checks */
		if (type == NotificationProblem && !notifiedProblemUsers->Contains(userName))
			notifiedProblemUsers->Add(userName);
	}

	/* if this was a recovery notification, reset all notified users */
	if (type == NotificationRecovery)
		notifiedProblemUsers->Clear();

	/* used in db_ido for notification history */
	Service::OnNotificationSentToAllUsers(this, checkable, allNotifiedUsers, type, cr, author, text, nullptr);
}

bool Notification::CheckNotificationUserFilters(NotificationType type, const User::Ptr& user, bool force, bool reminder)
{
	if (!force) {
		TimePeriod::Ptr tp = user->GetPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogNotice, "Notification")
				<< "Not sending " << (reminder ? "reminder " : " ") << "notifications for notification object '"
				<< GetName() << " and user '" << user->GetName()
				<< "': user period not in timeperiod '" << tp->GetName() << "'";
			return false;
		}

		unsigned long ftype = type;

		Log(LogDebug, "Notification")
			<< "User notification, Type '" << NotificationTypeToStringInternal(type)
			<< "', TypeFilter: " << NotificationFilterToString(user->GetTypeFilter(), GetTypeFilterMap())
			<< " (FType=" << ftype << ", TypeFilter=" << GetTypeFilter() << ")";


		if (!(ftype & user->GetTypeFilter())) {
			Log(LogNotice, "Notification")
				<< "Not sending " << (reminder ? "reminder " : " ") << "notifications for notification object '"
				<< GetName() << " and user '" << user->GetName() << "': type '"
				<< NotificationTypeToStringInternal(type) << "' does not match type filter: "
				<< NotificationFilterToString(user->GetTypeFilter(), GetTypeFilterMap()) << ".";
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
				<< "User notification, State '" << stateStr << "', StateFilter: "
				<< NotificationFilterToString(user->GetStateFilter(), GetStateFilterMap())
				<< " (FState=" << fstate << ", StateFilter=" << user->GetStateFilter() << ")";

			if (!(fstate & user->GetStateFilter())) {
				Log(LogNotice, "Notification")
					<< "Not " << (reminder ? "reminder " : " ") << "sending notifications for notification object '"
					<< GetName() << " and user '" << user->GetName() << "': state '" << stateStr
					<< "' does not match state filter: " << NotificationFilterToString(user->GetStateFilter(), GetStateFilterMap()) << ".";
				return false;
			}
		}
	} else {
		Log(LogNotice, "Notification")
			<< "Not checking " << (reminder ? "reminder " : " ") << "notification filters for notification object '"
			<< GetName() << "' and user '" << user->GetName() << "': Notification was forced.";
	}

	return true;
}

void Notification::ExecuteNotificationHelper(NotificationType type, const User::Ptr& user, const CheckResult::Ptr& cr, bool force, const String& author, const String& text)
{
	try {
		NotificationCommand::Ptr command = GetCommand();

		if (!command) {
			Log(LogDebug, "Notification")
				<< "No command found for notification '" << GetName() << "'. Skipping execution.";
			return;
		}

		command->Execute(this, user, cr, type, author, text);

		/* required by compatlogger */
		Service::OnNotificationSentToUser(this, GetCheckable(), user, type, cr, author, text, command->GetName(), nullptr);

		Log(LogInformation, "Notification")
			<< "Completed sending '" << NotificationTypeToStringInternal(type)
			<< "' notification '" << GetName()
			<< "' for checkable '" << GetCheckable()->GetName()
			<< "' and user '" << user->GetName() << "'.";
	} catch (const std::exception& ex) {
		Log(LogWarning, "Notification")
			<< "Exception occurred during notification for checkable '"
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

String Notification::NotificationFilterToString(int filter, const std::map<String, int>& filterMap)
{
	std::vector<String> sFilters;

	typedef std::pair<String, int> kv_pair;
	for (const kv_pair& kv : filterMap) {
		if (filter & kv.second)
			sFilters.push_back(kv.first);
	}

	return Utility::NaturalJoin(sFilters);
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

void Notification::ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Notification>::ValidateStates(lvalue, utils);

	int filter = FilterArrayToInt(lvalue(), GetStateFilterMap(), 0);

	if (GetServiceName().IsEmpty() && (filter == -1 || (filter & ~(StateFilterUp | StateFilterDown)) != 0))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "states" }, "State filter is invalid."));

	if (!GetServiceName().IsEmpty() && (filter == -1 || (filter & ~(StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown)) != 0))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "states" }, "State filter is invalid."));
}

void Notification::ValidateTypes(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Notification>::ValidateTypes(lvalue, utils);

	int filter = FilterArrayToInt(lvalue(), GetTypeFilterMap(), 0);

	if (filter == -1 || (filter & ~(NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved |
		NotificationCustom | NotificationAcknowledgement | NotificationProblem | NotificationRecovery |
		NotificationFlappingStart | NotificationFlappingEnd)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "types" }, "Type filter is invalid."));
}

Endpoint::Ptr Notification::GetCommandEndpoint() const
{
	return Endpoint::GetByName(GetCommandEndpointRaw());
}

const std::map<String, int>& Notification::GetStateFilterMap()
{
	return m_StateFilterMap;
}

const std::map<String, int>& Notification::GetTypeFilterMap()
{
	return m_TypeFilterMap;
}
