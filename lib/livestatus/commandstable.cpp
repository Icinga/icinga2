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

#include "livestatus/commandstable.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/compatutility.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

CommandsTable::CommandsTable()
{
	AddColumns(this);
}

void CommandsTable::AddColumns(Table *table, const String& prefix,
	const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "name", Column(&CommandsTable::NameAccessor, objectAccessor));
	table->AddColumn(prefix + "line", Column(&CommandsTable::LineAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_names", Column(&CommandsTable::CustomVariableNamesAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variable_values", Column(&CommandsTable::CustomVariableValuesAccessor, objectAccessor));
	table->AddColumn(prefix + "custom_variables", Column(&CommandsTable::CustomVariablesAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes", Column(&Table::ZeroAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&Table::ZeroAccessor, objectAccessor));
}

String CommandsTable::GetName() const
{
	return "commands";
}

String CommandsTable::GetPrefix() const
{
	return "command";
}

void CommandsTable::FetchRows(const AddRowFunction& addRowFn)
{
	for (const ConfigObject::Ptr& object : ConfigType::GetObjectsByType<CheckCommand>()) {
		if (!addRowFn(object, LivestatusGroupByNone, Empty))
			return;
	}

	for (const ConfigObject::Ptr& object : ConfigType::GetObjectsByType<EventCommand>()) {
		if (!addRowFn(object, LivestatusGroupByNone, Empty))
			return;
	}

	for (const ConfigObject::Ptr& object : ConfigType::GetObjectsByType<NotificationCommand>()) {
		if (!addRowFn(object, LivestatusGroupByNone, Empty))
			return;
	}
}

Value CommandsTable::NameAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	return CompatUtility::GetCommandName(command);
}

Value CommandsTable::LineAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	if (!command)
		return Empty;

	return CompatUtility::GetCommandLine(command);
}

Value CommandsTable::CustomVariableNamesAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	if (!command)
		return Empty;

	Dictionary::Ptr vars;

	{
		ObjectLock olock(command);
		vars = CompatUtility::GetCustomAttributeConfig(command);
	}

	Array::Ptr cv = new Array();

	if (!vars)
		return cv;

	{
		ObjectLock xlock(vars);
		for (const auto& kv : vars) {
			cv->Add(kv.first);
		}
	}

	return cv;
}

Value CommandsTable::CustomVariableValuesAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	if (!command)
		return Empty;

	Dictionary::Ptr vars;

	{
		ObjectLock olock(command);
		vars = CompatUtility::GetCustomAttributeConfig(command);
	}

	Array::Ptr cv = new Array();

	if (!vars)
		return cv;

	{
		ObjectLock xlock(vars);
		for (const auto& kv : vars) {
			cv->Add(kv.second);
		}
	}

	return cv;
}

Value CommandsTable::CustomVariablesAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	if (!command)
		return Empty;

	Dictionary::Ptr vars;

	{
		ObjectLock olock(command);
		vars = CompatUtility::GetCustomAttributeConfig(command);
	}

	Array::Ptr cv = new Array();

	if (!vars)
		return cv;

	{
		ObjectLock xlock(vars);
		for (const auto& kv : vars) {
			Array::Ptr key_val = new Array();
			key_val->Add(kv.first);
			key_val->Add(kv.second);
			cv->Add(key_val);
		}
	}

	return cv;
}
