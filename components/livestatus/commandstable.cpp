/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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
