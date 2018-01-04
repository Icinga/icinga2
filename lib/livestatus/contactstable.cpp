/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "livestatus/contactstable.hpp"
#include "icinga/user.hpp"
#include "icinga/timeperiod.hpp"
#include "icinga/compatutility.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/json.hpp"
#include "base/utility.hpp"
#include <boost/tuple/tuple.hpp>

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
	for (const User::Ptr& user : ConfigType::GetObjectsByType<User>()) {
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

	Dictionary::Ptr vars;

	{
		ObjectLock olock(user);
		vars = CompatUtility::GetCustomAttributeConfig(user);
	}

	Array::Ptr cv = new Array();

	if (!vars)
		return cv;

	ObjectLock olock(vars);
	for (const Dictionary::Pair& kv : vars) {
		cv->Add(kv.first);
	}

	return cv;
}

Value ContactsTable::CustomVariableValuesAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	Dictionary::Ptr vars;

	{
		ObjectLock olock(user);
		vars = CompatUtility::GetCustomAttributeConfig(user);
	}

	Array::Ptr cv = new Array();

	if (!vars)
		return cv;

	ObjectLock olock(vars);
	for (const Dictionary::Pair& kv : vars) {
		if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
			cv->Add(JsonEncode(kv.second));
		else
			cv->Add(kv.second);
	}

	return cv;
}

Value ContactsTable::CustomVariablesAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	Dictionary::Ptr vars;

	{
		ObjectLock olock(user);
		vars = CompatUtility::GetCustomAttributeConfig(user);
	}

	Array::Ptr cv = new Array();

	if (!vars)
		return cv;

	ObjectLock olock(vars);
	for (const Dictionary::Pair& kv : vars) {
		Array::Ptr key_val = new Array();
		key_val->Add(kv.first);

		if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
			key_val->Add(JsonEncode(kv.second));
		else
			key_val->Add(kv.second);

		cv->Add(key_val);
	}

	return cv;
}

Value ContactsTable::CVIsJsonAccessor(const Value& row)
{
	User::Ptr user = static_cast<User::Ptr>(row);

	if (!user)
		return Empty;

	Dictionary::Ptr vars;

	{
		ObjectLock olock(user);
		vars = CompatUtility::GetCustomAttributeConfig(user);
	}

	if (!vars)
		return Empty;

	bool cv_is_json = false;

	ObjectLock olock(vars);
	for (const Dictionary::Pair& kv : vars) {
		if (kv.second.IsObjectType<Array>() || kv.second.IsObjectType<Dictionary>())
			cv_is_json = true;
	}

	return cv_is_json;
}
