/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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
#include <boost/foreach.hpp>

using namespace icinga;
using namespace livestatus;

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
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("CheckCommand")) {
		addRowFn(object);
	}
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("EventCommand")) {
		addRowFn(object);
	}
	BOOST_FOREACH(const DynamicObject::Ptr& object, DynamicType::GetObjects("NotificationCommand")) {
		addRowFn(object);
	}
}

Value CommandsTable::NameAccessor(const Value& row)
{
	/* TODO */
	return Value();
}

Value CommandsTable::LineAccessor(const Value& row)
{
	/* TODO */
	return Value();
}
