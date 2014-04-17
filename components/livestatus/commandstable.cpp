/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "livestatus/commandstable.h"
#include "icinga/icingaapplication.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/notificationcommand.h"
#include "icinga/compatutility.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/convert.h"
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
	return "command";
}

void CommandsTable::FetchRows(const AddRowFunction& addRowFn)
{
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects<CheckCommand>()) {
		addRowFn(object);
	}
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects<EventCommand>()) {
		addRowFn(object);
	}
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects<NotificationCommand>()) {
		addRowFn(object);
	}
}

Value CommandsTable::NameAccessor(const Value& row)
{
	String buf;
	Command::Ptr command = static_cast<Command::Ptr>(row);
	
	if (!command)
		return Empty;

	if (command->GetType() == DynamicType::GetByName("CheckCommand"))
		buf += "check_";
	if (command->GetType() == DynamicType::GetByName("NotificationCommand"))
		buf += "notification_";
	if (command->GetType() == DynamicType::GetByName("EventCommand"))
		buf += "event_";

	buf += command->GetName();

	return buf;
}

Value CommandsTable::LineAccessor(const Value& row)
{
	String buf;
	Command::Ptr command = static_cast<Command::Ptr>(row);
	
	if (!command)
		return Empty;

	Value commandLine = command->GetCommandLine();

	if (commandLine.IsObjectType<Array>()) {
		Array::Ptr args = commandLine;

		ObjectLock olock(args);
		String arg;
		BOOST_FOREACH(arg, args) {
			// This is obviously incorrect for non-trivial cases.
			String argitem = " \"" + arg + "\"";
			boost::algorithm::replace_all(argitem, "\n", "\\n");
			buf += argitem;
		}
	} else if (!commandLine.IsEmpty()) {
		String args = Convert::ToString(commandLine);
		boost::algorithm::replace_all(args, "\n", "\\n");
		buf += args;
	} else {
		buf += "<internal>";
	}

	return buf;
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

	Array::Ptr cv = make_shared<Array>();

	String key;
	Value value;
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

	Array::Ptr cv = make_shared<Array>();

	String key;
	Value value;
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

	Array::Ptr cv = make_shared<Array>();

	String key;
	Value value;
	BOOST_FOREACH(tie(key, value), vars) {
		Array::Ptr key_val = make_shared<Array>();
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
