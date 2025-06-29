/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
#include <algorithm>

using namespace icinga;

REGISTER_TYPE(Notification);
INITIALIZE_ONCE(&Notification::StaticInitialize);

std::map<String, int> Notification::m_StateFilterMap;
std::map<String, int> Notification::m_TypeFilterMap;

boost::signals2::signal<void (const Notification::Ptr&, const MessageOrigin::Ptr&)> Notification::OnNextNotificationChanged;
boost::signals2::signal<void (const Notification::Ptr&, const String&, uint_fast8_t, const MessageOrigin::Ptr&)> Notification::OnLastNotifiedStatePerUserUpdated;
boost::signals2::signal<void (const Notification::Ptr&, const MessageOrigin::Ptr&)> Notification::OnLastNotifiedStatePerUserCleared;

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
	ScriptGlobal::Set("Icinga.OK", "OK");
	ScriptGlobal::Set("Icinga.Warning", "Warning");
	ScriptGlobal::Set("Icinga.Critical", "Critical");
	ScriptGlobal::Set("Icinga.Unknown", "Unknown");
	ScriptGlobal::Set("Icinga.Up", "Up");
	ScriptGlobal::Set("Icinga.Down", "Down");

	ScriptGlobal::Set("Icinga.DowntimeStart", "DowntimeStart");
	ScriptGlobal::Set("Icinga.DowntimeEnd", "DowntimeEnd");
	ScriptGlobal::Set("Icinga.DowntimeRemoved", "DowntimeRemoved");
	ScriptGlobal::Set("Icinga.Custom", "Custom");
	ScriptGlobal::Set("Icinga.Acknowledgement", "Acknowledgement");
	ScriptGlobal::Set("Icinga.Problem", "Problem");
	ScriptGlobal::Set("Icinga.Recovery", "Recovery");
	ScriptGlobal::Set("Icinga.FlappingStart", "FlappingStart");
	ScriptGlobal::Set("Icinga.FlappingEnd", "FlappingEnd");

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

Notification::Notification()
{
	// If a notification is created without specifying the "types/states" attribute, the Set* methods won't be called,
	// consequently the filter bitset will also be 0. Thus, we need to ensure that the type/state filter are
	// initialized to the default values, which are all types and states enabled.
	SetTypes(nullptr, false, Empty);
	SetStates(nullptr, false, Empty);
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

	for (const UserGroup::Ptr& group : GetUserGroups())
		group->AddNotification(this);

	ObjectImpl<Notification>::Start(runtimeCreated);
}

void Notification::Stop(bool runtimeRemoved)
{
	ObjectImpl<Notification>::Stop(runtimeRemoved);

	Checkable::Ptr obj = GetCheckable();

	if (obj)
		obj->UnregisterNotification(this);

	for (const UserGroup::Ptr& group : GetUserGroups())
		group->RemoveNotification(this);
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

		for (String name : users) {
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

		for (String name : groups) {
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

/**
 * Check whether the given notification type is a Recovery or FlappingEnd notification and the Checkable object
 * has already recovered.
 *
 * For the latter case, if the Checkable has recovered while it was in a Flapping state, the recovery notification
 * will be silently discarded and leave some internal states of the Notification object that depend on it in an
 * inconsistent state. So, use this helper function whether to reset one of these states.
 *
 * @param checkable The checkable object the notification is for
 * @param cr The current check result passed to the notification
 * @param type The requested notification type
 *
 * @return bool
 */
static bool IsRecoveryOrFlappingEndAndCheckableIsOK(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, NotificationType type)
{
	if (type == NotificationRecovery) {
		return true;
	}

	// Check whether missed the Checkable recovery because of its Flapping state.
	return type == NotificationFlappingEnd && cr && checkable->IsStateOK(cr->GetState());
}

void Notification::BeginExecuteNotification(NotificationType type, const CheckResult::Ptr& cr, bool force, bool reminder, const String& author, const String& text)
{
	String notificationName = GetName();
	String notificationTypeName = NotificationTypeToString(type);

	Log(LogNotice, "Notification")
		<< "Attempting to send " << (reminder ? "reminder " : "")
		<< "notifications of type '" << notificationTypeName
		<< "' for notification object '" << notificationName << "'.";

	Checkable::Ptr checkable = GetCheckable();

	// Clear the last notified problem state per user if we're sending a recovery notification or if we're sending a
	// flapping end notification and the checkable is already in an OK state. This is necessary since we might have
	// missed the recovery notification due to the flapping state.
	if (IsRecoveryOrFlappingEndAndCheckableIsOK(checkable, cr, type)) {
		auto states (GetLastNotifiedStatePerUser());

		states->Clear();
		OnLastNotifiedStatePerUserCleared(this, nullptr);
	}

	if (!force) {
		TimePeriod::Ptr tp = GetPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogNotice, "Notification")
				<< "Not sending " << (reminder ? "reminder " : "") << "notifications for notification object '" << notificationName
				<< "': not in timeperiod '" << tp->GetName() << "'";

			if (!reminder) {
				switch (type) {
					case NotificationProblem:
					case NotificationRecovery:
					case NotificationFlappingStart:
					case NotificationFlappingEnd:
						{
							/* If a non-reminder notification was suppressed, but just because of its time period,
							 * stash it into a notification types bitmask for maybe re-sending later.
							 */

							ObjectLock olock (this);
							int suppressedTypesBefore (GetSuppressedNotifications());
							int suppressedTypesAfter (suppressedTypesBefore | type);

							for (int conflict : {NotificationProblem | NotificationRecovery, NotificationFlappingStart | NotificationFlappingEnd}) {
								/* E.g. problem and recovery notifications neutralize each other. */

								if ((suppressedTypesAfter & conflict) == conflict) {
									suppressedTypesAfter &= ~conflict;
								}
							}

							if (suppressedTypesAfter != suppressedTypesBefore) {
								SetSuppressedNotifications(suppressedTypesAfter);
							}
						}
					default:
						; // Cheating the compiler on "5 enumeration values not handled in switch"
				}
			}

			return;
		}

		double now = Utility::GetTime();
		Dictionary::Ptr times = GetTimes();

		if (times && type == NotificationProblem) {
			Value timesBegin = times->Get("begin");
			Value timesEnd = times->Get("end");

			if (timesBegin != Empty && timesBegin >= 0 && now < checkable->GetLastHardStateChange() + timesBegin) {
				Log(LogNotice, "Notification")
					<< "Not sending " << (reminder ? "reminder " : "") << "notifications for notification object '"
					<< notificationName << "': before specified begin time (" << Utility::FormatDuration(timesBegin) << ")";

				/* we need to adjust the next notification time
				 * delaying the first notification
				 */
				SetNextNotification(checkable->GetLastHardStateChange() + timesBegin + 1.0);

				/*
				 * We need to set no more notifications to false, in case
				 * some notifications were sent previously
				 */
				SetNoMoreNotifications(false);

				return;
			}

			if (timesEnd != Empty && timesEnd >= 0 && now > checkable->GetLastHardStateChange() + timesEnd) {
				Log(LogNotice, "Notification")
					<< "Not sending " << (reminder ? "reminder " : "") << "notifications for notification object '"
					<< notificationName << "': after specified end time (" << Utility::FormatDuration(timesEnd) << ")";
				return;
			}
		}

		unsigned long ftype = type;

		Log(LogDebug, "Notification")
			<< "Type '" << NotificationTypeToString(type)
			<< "', TypeFilter: " << NotificationFilterToString(GetTypeFilter(), GetTypeFilterMap())
			<< " (FType=" << ftype << ", TypeFilter=" << GetTypeFilter() << ")";

		if (!(ftype & GetTypeFilter())) {
			Log(LogNotice, "Notification")
				<< "Not sending " << (reminder ? "reminder " : "") << "notifications for notification object '"
				<< notificationName << "': type '"
				<< NotificationTypeToString(type) << "' does not match type filter: "
				<< NotificationFilterToString(GetTypeFilter(), GetTypeFilterMap()) << ".";

			/* Ensure to reset no_more_notifications on Recovery notifications,
			 * even if the admin did not configure them in the filter.
			 */
			{
				ObjectLock olock(this);
				if (GetInterval() <= 0 && IsRecoveryOrFlappingEndAndCheckableIsOK(checkable, cr, type))
					SetNoMoreNotifications(false);
			}

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
					<< "Not sending " << (reminder ? "reminder " : "") << "notifications for notification object '"
					<< notificationName << "': state '" << stateStr
					<< "' does not match state filter: " << NotificationFilterToString(GetStateFilter(), GetStateFilterMap()) << ".";
				return;
			}
		}
	} else {
		Log(LogNotice, "Notification")
			<< "Not checking " << (reminder ? "reminder " : "") << "notification filters for notification object '"
			<< notificationName << "': Notification was forced.";
	}

	{
		ObjectLock olock(this);

		UpdateNotificationNumber();
		double now = Utility::GetTime();
		SetLastNotification(now);

		if (type == NotificationProblem && GetInterval() <= 0)
			SetNoMoreNotifications(true);
		else if (IsRecoveryOrFlappingEndAndCheckableIsOK(checkable, cr, type))
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
				<< "Notification object '" << notificationName << "': Disabled notifications for user '"
				<< userName << "'. Not sending notification.";
			continue;
		}

		if (!CheckNotificationUserFilters(type, user, force, reminder)) {
			Log(LogNotice, "Notification")
				<< "Notification object '" << notificationName << "': Filters for user '" << userName << "' not matched. Not sending notification.";
			continue;
		}

		/* on acknowledgement/recovery, check if user was notified before */
		if (type == NotificationAcknowledgement || type == NotificationRecovery) {
			// Do not notify the user about the ACK/recovery of a problem state that they have not been notified
			// about, unless they are explicitly configured to receive ACK/recovery notifications but no problem
			// ones, or this Notification instance isn't allowed to send problem notifications (doesn't contain
			// the 'Problem' type filter).
			if (!notifiedProblemUsers->Contains(userName) && NotificationProblem & user->GetTypeFilter() & GetTypeFilter()) {
				Log(LogNotice, "Notification")
					<< "Notification object '" << notificationName << "': We did not notify user '" << userName
					<< "' (Problem types enabled) for a problem before. Not sending "
					<< (type == NotificationRecovery ? "Recovery" : "acknowledgement") << " notification.";
				continue;
			}
		}

		if (type == NotificationProblem && !reminder && !checkable->GetVolatile()) {
			auto [host, service] = GetHostService(checkable);
			uint_fast8_t state = service ? static_cast<uint_fast8_t>(service->GetState())
				: static_cast<uint_fast8_t>(host->GetState());

			if (state == (uint_fast8_t)GetLastNotifiedStatePerUser()->Get(userName)) {
				auto stateStr (service ? NotificationServiceStateToString(service->GetState()) : NotificationHostStateToString(host->GetState()));

				Log(LogNotice, "Notification")
					<< "Notification object '" << notificationName << "': We already notified user '" << userName << "' for a " << stateStr
					<< " problem. Likely after that another state change notification was filtered out by config. Not sending duplicate '"
					<< stateStr << "' notification.";

				continue;
			}
		}

		Log(LogInformation, "Notification")
			<< "Sending " << (reminder ? "reminder " : "") << "'" << NotificationTypeToString(type) << "' notification '"
			<< notificationName << "' for user '" << userName << "'";

		// Explicitly use Notification::Ptr to keep the reference counted while the callback is active
		Notification::Ptr notification (this);
		Utility::QueueAsyncCallback([notification, type, user, cr, force, author, text]() {
			notification->ExecuteNotificationHelper(type, user, cr, force, author, text);
		});

		/* collect all notified users */
		allNotifiedUsers.insert(user);

		if (type == NotificationProblem) {
			auto [host, service] = GetHostService(checkable);
			uint_fast8_t state = service ? static_cast<uint_fast8_t>(service->GetState())
				: static_cast<uint_fast8_t>(host->GetState());

			if (state != (uint_fast8_t)GetLastNotifiedStatePerUser()->Get(userName)) {
				GetLastNotifiedStatePerUser()->Set(userName, state);
				OnLastNotifiedStatePerUserUpdated(this, userName, state, nullptr);
			}
		}

		/* store all notified users for later recovery checks */
		if (type == NotificationProblem && !notifiedProblemUsers->Contains(userName))
			notifiedProblemUsers->Add(userName);
	}

	/* if this was a recovery notification, reset all notified users */
	if (IsRecoveryOrFlappingEndAndCheckableIsOK(checkable, cr, type))
		notifiedProblemUsers->Clear();

	/* used in db_ido for notification history */
	Service::OnNotificationSentToAllUsers(this, checkable, allNotifiedUsers, type, cr, author, text, nullptr);
}

bool Notification::CheckNotificationUserFilters(NotificationType type, const User::Ptr& user, bool force, bool reminder)
{
	String notificationName = GetName();
	String userName = user->GetName();

	if (!force) {
		TimePeriod::Ptr tp = user->GetPeriod();

		if (tp && !tp->IsInside(Utility::GetTime())) {
			Log(LogNotice, "Notification")
				<< "Not sending " << (reminder ? "reminder " : "") << "notifications for notification object '"
				<< notificationName << " and user '" << userName
				<< "': user period not in timeperiod '" << tp->GetName() << "'";
			return false;
		}

		unsigned long ftype = type;

		Log(LogDebug, "Notification")
			<< "User '" << userName << "' notification '" << notificationName
			<< "', Type '" << NotificationTypeToString(type)
			<< "', TypeFilter: " << NotificationFilterToString(user->GetTypeFilter(), GetTypeFilterMap())
			<< " (FType=" << ftype << ", TypeFilter=" << GetTypeFilter() << ")";


		if (!(ftype & user->GetTypeFilter())) {
			Log(LogNotice, "Notification")
				<< "Not sending " << (reminder ? "reminder " : "") << "notifications for notification object '"
				<< notificationName << " and user '" << userName << "': type '"
				<< NotificationTypeToString(type) << "' does not match type filter: "
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
				<< "User '" << userName << "' notification '" << notificationName
				<< "', State '" << stateStr << "', StateFilter: "
				<< NotificationFilterToString(user->GetStateFilter(), GetStateFilterMap())
				<< " (FState=" << fstate << ", StateFilter=" << user->GetStateFilter() << ")";

			if (!(fstate & user->GetStateFilter())) {
				Log(LogNotice, "Notification")
					<< "Not " << (reminder ? "reminder " : "") << "sending notifications for notification object '"
					<< notificationName << " and user '" << userName << "': state '" << stateStr
					<< "' does not match state filter: " << NotificationFilterToString(user->GetStateFilter(), GetStateFilterMap()) << ".";
				return false;
			}
		}
	} else {
		Log(LogNotice, "Notification")
			<< "Not checking " << (reminder ? "reminder " : "") << "notification filters for notification object '"
			<< notificationName << "' and user '" << userName << "': Notification was forced.";
	}

	return true;
}

void Notification::ExecuteNotificationHelper(NotificationType type, const User::Ptr& user, const CheckResult::Ptr& cr, bool force, const String& author, const String& text)
{
	String notificationName = GetName();
	String userName = user->GetName();
	String checkableName = GetCheckable()->GetName();

	NotificationCommand::Ptr command = GetCommand();

	if (!command) {
		Log(LogDebug, "Notification")
			<< "No command found for notification '" << notificationName << "'. Skipping execution.";
		return;
	}

	String commandName = command->GetName();

	try {
		command->Execute(this, user, cr, type, author, text);

		/* required by compatlogger */
		Service::OnNotificationSentToUser(this, GetCheckable(), user, type, cr, author, text, commandName, nullptr);

		Log(LogInformation, "Notification")
			<< "Completed sending '" << NotificationTypeToString(type)
			<< "' notification '" << notificationName
			<< "' for checkable '" << checkableName
			<< "' and user '" << userName << "' using command '" << commandName << "'.";
	} catch (const std::exception& ex) {
		Log(LogWarning, "Notification")
			<< "Exception occurred during notification '" << notificationName
			<< "' for checkable '" << checkableName
			<< "' and user '" << userName << "' using command '" << commandName << "': "
			<< DiagnosticInformation(ex, false);
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

	for (auto& kv : filterMap) {
		if (filter & kv.second)
			sFilters.push_back(kv.first);
	}

	return Utility::NaturalJoin(sFilters);
}

/*
 * Main interface to translate NotificationType values into strings.
 */
String Notification::NotificationTypeToString(NotificationType type)
{
	auto typeMap = Notification::m_TypeFilterMap;

	auto it = std::find_if(typeMap.begin(), typeMap.end(),
		[&type](const std::pair<String, int>& p) {
			return p.second == type;
	});

	if (it == typeMap.end())
		return Empty;

	return it->first;
}


/*
 * Compat interface used in external features.
 */
String Notification::NotificationTypeToStringCompat(NotificationType type)
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

Array::Ptr Notification::GetTypes() const
{
	return m_Types.load();
}

void Notification::SetTypes(const Array::Ptr& value, bool suppress_events, const Value& cookie)
{
	m_Types.store(value);
	// Ensure that the type filter is updated when the types attribute changes.
	SetTypeFilter(FilterArrayToInt(value, GetTypeFilterMap(), ~0));
	if (!suppress_events) {
		NotifyTypes(cookie);
	}
}

Array::Ptr Notification::GetStates() const
{
	return m_States.load();
}

void Notification::SetStates(const Array::Ptr& value, bool suppress_events, const Value& cookie)
{
	m_States.store(value);
	// Ensure that the state filter is updated when the states attribute changes.
	SetStateFilter(FilterArrayToInt(value, GetStateFilterMap(), ~0));
	if (!suppress_events) {
		NotifyStates(cookie);
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
		BOOST_THROW_EXCEPTION(ValidationError(this, std::vector<String>(), "No users/user_groups specified."));
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

void Notification::ValidateTimes(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Notification>::ValidateTimes(lvalue, utils);

	Dictionary::Ptr times = lvalue();

	if (!times)
		return;

	double begin;
	double end;

	try {
		begin = Convert::ToDouble(times->Get("begin"));
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ValidationError(this, { "times" }, "'begin' is invalid, must be duration or number." ));
	}

	try {
		end = Convert::ToDouble(times->Get("end"));
	} catch (const std::exception&) {
		BOOST_THROW_EXCEPTION(ValidationError(this, { "times" }, "'end' is invalid, must be duration or number." ));
	}

	/* Also solve logical errors where begin > end. */
	if (begin > 0 && end > 0 && begin > end)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "times" }, "'begin' must be smaller than 'end'."));
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
