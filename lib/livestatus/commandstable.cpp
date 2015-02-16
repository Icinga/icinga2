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

#include "livestatus/commandstable.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "icinga/compatutility.hpp"
#include "base/dynamictype.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include <boost/foreach.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

CommandsTable::CommandsTable(void)
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
	table->AddColumn(prefix + "modified_attributes", Column(&CommandsTable::ModifiedAttributesAccessor, objectAccessor));
	table->AddColumn(prefix + "modified_attributes_list", Column(&CommandsTable::ModifiedAttributesListAccessor, objectAccessor));
}

String CommandsTable::GetName(void) const
{
	return "commands";
}

String CommandsTable::GetPrefix(void) const
{
	return "command";
}

void CommandsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjectsByType<CheckCommand>()) {
		addRowFn(object);
	}
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjectsByType<EventCommand>()) {
		addRowFn(object);
	}
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjectsByType<NotificationCommand>()) {
		addRowFn(object);
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

	if (!vars)
		return Empty;

	Array::Ptr cv = new Array();

	String key;
	Value value;

	ObjectLock xlock(vars);
	BOOST_FOREACH(tie(key, value), vars) {
		cv->Add(key);
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

	if (!vars)
		return Empty;

	Array::Ptr cv = new Array();

	String key;
	Value value;

	ObjectLock xlock(vars);
	BOOST_FOREACH(tie(key, value), vars) {
		cv->Add(value);
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

	if (!vars)
		return Empty;

	Array::Ptr cv = new Array();

	String key;
	Value value;

	ObjectLock xlock(vars);
	BOOST_FOREACH(tie(key, value), vars) {
		Array::Ptr key_val = new Array();
		key_val->Add(key);
		key_val->Add(value);
		cv->Add(key_val);
	}

	return cv;
}

Value CommandsTable::ModifiedAttributesAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	if (!command)
		return Empty;

	/* not supported */
	return command->GetModifiedAttributes();
}

Value CommandsTable::ModifiedAttributesListAccessor(const Value& row)
{
	Command::Ptr command = static_cast<Command::Ptr>(row);

	if (!command)
		return Empty;

	return CompatUtility::GetModifiedAttributesList(command);
}
