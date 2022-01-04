/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "livestatus/contactstable.hpp"
#include "icinga/user.hpp"
#include "icinga/timeperiod.hpp"
#include "icinga/compatutility.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include "base/utility.hpp"

using namespace icinga;

ContactsTable::ContactsTable()
{
	AddColumns(this);
}

void ContactsTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&ContactsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "alias", Column(&ContactsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "email", Column(&ContactsTable::EmailAccessor, objectAccessor));
	table->AddColumn(prefix + "pager", Column(&ContactsTable::PagerAccessor, objectAccessor));
	table->AddColumn(prefix + "host_notification_period", Column(&ContactsTable::HostNotificationPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "service_notification_period", Column(&ContactsTable::ServiceNotificationPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "can_submit_commands", Column(&Table::OneAccessor, objectAccessor));
	table->AddColumn(prefix + "host_notifications_enabled", Column(&ContactsTable::HostNotificationsEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "service_notifications_enabled", Column(&ContactsTable::ServiceNotificationsEnabledAccessor, objectAccessor));
	table->AddColumn(prefix + "in_host_notification_period", Column(&ContactsTable::InHostNotificationPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "in_service_notification_period", Column(&ContactsTable::InServiceNotificationPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "vars_variable_names", Column(&ContactsTable::CustomVariableNamesAccessor, objectAccessor));
	table->AddColumn(prefix + "vars_variable_values", Column(&ContactsTable::CustomVariableValuesAccessor, objectAccessor));
	table->AddColumn(prefix + "vars_variables", Column(&ContactsTable::CustomVariablesAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "cv_is_json", Column(&ContactsTable::CVIsJsonAccessor, objectAccessor));

}

String ContactsTable::GetName() const
{
	return "contacts";
}

String ContactsTable::GetPrefix() const
{
	return "contact";
}

void ContactsTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const auto& user : ConfigType::GetObjectsByType<User>()) {
		if (!addRowFn(user, LivestatusGroupByNone, Empty))
			return;
	}
}

Value ContactsTable::NameAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	return user->GetName();
}

Value ContactsTable::AliasAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	return user->GetDisplayName();
}

Value ContactsTable::EmailAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	return user->GetEmail();
}

Value ContactsTable::PagerAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	return user->GetPager();
}

Value ContactsTable::HostNotificationPeriodAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	/* same as service */
	TimePeriod::Ptr timeperiod = user->GetPeriod();

	if (!timeperiod)
		return Empty;

	return timeperiod->GetName();
}

Value ContactsTable::ServiceNotificationPeriodAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	TimePeriod::Ptr timeperiod = user->GetPeriod();

	if (!timeperiod)
		return Empty;

	return timeperiod->GetName();
}

Value ContactsTable::HostNotificationsEnabledAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	return (user->GetEnableNotifications() ? 1 : 0);
}

Value ContactsTable::ServiceNotificationsEnabledAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	return (user->GetEnableNotifications() ? 1 : 0);
}

Value ContactsTable::InHostNotificationPeriodAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	TimePeriod::Ptr timeperiod = user->GetPeriod();

	if (!timeperiod)
		return Empty;

	return (timeperiod->IsInside(Utility::GetTime()) ? 1 : 0);
}

Value ContactsTable::InServiceNotificationPeriodAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	TimePeriod::Ptr timeperiod = user->GetPeriod();

	if (!timeperiod)
		return Empty;

	return (timeperiod->IsInside(Utility::GetTime()) ? 1 : 0);
}

Value ContactsTable::CustomVariableNamesAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	Dictionary::Ptr vars = user->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock olock(vars);
		for (const auto& kv : vars) {
			result.push_back(kv.first);
		}
	}

	return new Array(std::move(result));
}

Value ContactsTable::CustomVariableValuesAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	Dictionary::Ptr vars = user->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock olock(vars);
		for (const auto& kv : vars) {
			if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
				result.push_back(JsonEncode(kv.second));
			else
				result.push_back(kv.second);
		}
	}

	return new Array(std::move(result));
}

Value ContactsTable::CustomVariablesAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	Dictionary::Ptr vars = user->GetVars();

	ArrayData result;

	if (vars) {
		ObjectLock olock(vars);
		for (const auto& kv : vars) {
			Value val;

			if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
				val = JsonEncode(kv.second);
			else
				val = kv.second;

			result.push_back(new Array({
				kv.first,
				val
			}));
		}
	}

	return new Array(std::move(result));
}

Value ContactsTable::CVIsJsonAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	Dictionary::Ptr vars = user->GetVars();

	if (!vars)
		return Empty;

	bool cv_is_json = false;

	ObjectLock olock(vars);
	for (const auto& kv : vars) {
		if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
			cv_is_json = true;
	}

	return cv_is_json;
}
