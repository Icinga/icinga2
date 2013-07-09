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

#include "livestatus/logtable.h"
#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
#include <boost/smart_ptr/make_shared.hpp>

using namespace icinga;
using namespace livestatus;

LogTable::LogTable(void)
{
	AddColumns(this);
}

void LogTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "time", Column(&LogTable::TimeAccessor, objectAccessor));
	table->AddColumn(prefix + "lineno", Column(&LogTable::LinenoAccessor, objectAccessor));
	table->AddColumn(prefix + "class", Column(&LogTable::ClassAccessor, objectAccessor));
	table->AddColumn(prefix + "message", Column(&LogTable::MessageAccessor, objectAccessor));
	table->AddColumn(prefix + "type", Column(&LogTable::TypeAccessor, objectAccessor));
	table->AddColumn(prefix + "options", Column(&LogTable::OptionsAccessor, objectAccessor));
	table->AddColumn(prefix + "comment", Column(&LogTable::CommentAccessor, objectAccessor));
	table->AddColumn(prefix + "plugin_output", Column(&LogTable::PluginOutputAccessor, objectAccessor));
	table->AddColumn(prefix + "state", Column(&LogTable::StateAccessor, objectAccessor));
	table->AddColumn(prefix + "state_type", Column(&LogTable::StateTypeAccessor, objectAccessor));
	table->AddColumn(prefix + "attempt", Column(&LogTable::AttemptAccessor, objectAccessor));
	table->AddColumn(prefix + "service_description", Column(&LogTable::ServiceDescriptionAccessor, objectAccessor));
	table->AddColumn(prefix + "host_name", Column(&LogTable::HostNameAccessor, objectAccessor));
	table->AddColumn(prefix + "contact_name", Column(&LogTable::ContactNameAccessor, objectAccessor));
	table->AddColumn(prefix + "command_name", Column(&LogTable::CommandNameAccessor, objectAccessor));

	// TODO join with hosts, services, contacts, command tables
}

String LogTable::GetName(void) const
{
	return "log";
}

void LogTable::FetchRows(const AddRowFunction& addRowFn)
{
	Object::Ptr obj = boost::make_shared<Object>();

	/* Return a fake row. */
	addRowFn(obj);
}

Value LogTable::TimeAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::LinenoAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::ClassAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::MessageAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::TypeAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::OptionsAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::CommentAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::PluginOutputAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::StateAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::StateTypeAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::AttemptAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::ServiceDescriptionAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::HostNameAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::ContactNameAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}

Value LogTable::CommandNameAccessor(const Object::Ptr& object)
{
	/* not supported */
	return Value();
}
