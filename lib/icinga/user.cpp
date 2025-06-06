/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/user.hpp"
#include "icinga/user-ti.cpp"
#include "icinga/usergroup.hpp"
#include "icinga/notification.hpp"
#include "icinga/usergroup.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"

using namespace icinga;

REGISTER_TYPE(User);

User::User()
{
	// If a User is created without specifying the "types/states" attribute, the Set* methods won't be called,
	// consequently the filter bitset will also be 0. Thus, we need to ensure that the type/state filter are
	// initialized to the default values, which are all types and states enabled.
	SetTypes(nullptr, false, Empty);
	SetStates(nullptr, false, Empty);
}

void User::OnAllConfigLoaded()
{
	ObjectImpl<User>::OnAllConfigLoaded();

	UserGroup::EvaluateObjectRules(this);

	Array::Ptr groups = GetGroups();

	if (groups) {
		groups = groups->ShallowClone();

		ObjectLock olock(groups);

		for (String name : groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (ug)
				ug->ResolveGroupMembership(this, true);
		}
	}
}

void User::Stop(bool runtimeRemoved)
{
	ObjectImpl<User>::Stop(runtimeRemoved);

	Array::Ptr groups = GetGroups();

	if (groups) {
		ObjectLock olock(groups);

		for (String name : groups) {
			UserGroup::Ptr ug = UserGroup::GetByName(name);

			if (ug)
				ug->ResolveGroupMembership(this, false);
		}
	}
}

void User::AddGroup(const String& name)
{
	std::unique_lock<std::mutex> lock(m_UserMutex);

	Array::Ptr groups = GetGroups();

	if (groups && groups->Contains(name))
		return;

	if (!groups)
		groups = new Array();

	groups->Add(name);
}

TimePeriod::Ptr User::GetPeriod() const
{
	return TimePeriod::GetByName(GetPeriodRaw());
}

Array::Ptr User::GetTypes() const
{
	return m_Types.load();
}

void User::SetTypes(const Array::Ptr& value, bool suppress_events, const Value& cookie)
 {
	m_Types.store(value);
	// Ensure that the type filter is updated when the types attribute changes.
	SetTypeFilter(FilterArrayToInt(value, Notification::GetTypeFilterMap(), ~0));
	if (!suppress_events) {
		NotifyTypes(cookie);
	}
}

Array::Ptr User::GetStates() const
{
	return m_States.load();
}

void User::SetStates(const Array::Ptr& value, bool suppress_events, const Value& cookie)
{
	m_States.store(value);
	// Ensure that the state filter is updated when the states attribute changes.
	SetStateFilter(FilterArrayToInt(value, Notification::GetStateFilterMap(), ~0));
	if (!suppress_events) {
		NotifyStates(cookie);
	}
}

void User::ValidateStates(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<User>::ValidateStates(lvalue, utils);

	int filter = FilterArrayToInt(lvalue(), Notification::GetStateFilterMap(), 0);

	if (filter == -1 || (filter & ~(StateFilterUp | StateFilterDown | StateFilterOK | StateFilterWarning | StateFilterCritical | StateFilterUnknown)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "states" }, "State filter is invalid."));
}

void User::ValidateTypes(const Lazy<Array::Ptr>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<User>::ValidateTypes(lvalue, utils);

	int filter = FilterArrayToInt(lvalue(), Notification::GetTypeFilterMap(), 0);

	if (filter == -1 || (filter & ~(NotificationDowntimeStart | NotificationDowntimeEnd | NotificationDowntimeRemoved |
		NotificationCustom | NotificationAcknowledgement | NotificationProblem | NotificationRecovery |
		NotificationFlappingStart | NotificationFlappingEnd)) != 0)
		BOOST_THROW_EXCEPTION(ValidationError(this, { "types" }, "Type filter is invalid."));
}
