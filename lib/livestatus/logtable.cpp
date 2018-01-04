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

#include "livestatus/logtable.hpp"
#include "livestatus/livestatuslogutility.hpp"
#include "livestatus/hoststable.hpp"
#include "livestatus/servicestable.hpp"
#include "livestatus/contactstable.hpp"
#include "livestatus/commandstable.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/cib.hpp"
#include "icinga/service.hpp"
#include "icinga/host.hpp"
#include "icinga/user.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/logger.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>

using namespace icinga;

LogTable::LogTable(const String& compat_log_path, time_t from, time_t until)
{
	/* store attributes for FetchRows */
	m_TimeFrom = from;
	m_TimeUntil = until;
	m_CompatLogPath = compat_log_path;

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

	HostsTable::AddColumns(table, "current_host_", std::bind(&LogTable::HostAccessor, _1, objectAccessor));
	ServicesTable::AddColumns(table, "current_service_", std::bind(&LogTable::ServiceAccessor, _1, objectAccessor));
	ContactsTable::AddColumns(table, "current_contact_", std::bind(&LogTable::ContactAccessor, _1, objectAccessor));
	CommandsTable::AddColumns(table, "current_command_", std::bind(&LogTable::CommandAccessor, _1, objectAccessor));
}

String LogTable::GetName() const
{
	return "log";
}

String LogTable::GetPrefix() const
{
	return "log";
}

void LogTable::FetchRows(const AddRowFunction& addRowFn)
{
	Log(LogDebug, "LogTable")
		<< "Pre-selecting log file from " << m_TimeFrom << " until " << m_TimeUntil;

	/* create log file index */
	LivestatusLogUtility::CreateLogIndex(m_CompatLogPath, m_LogFileIndex);

	/* generate log cache */
	LivestatusLogUtility::CreateLogCache(m_LogFileIndex, this, m_TimeFrom, m_TimeUntil, addRowFn);
}

/* gets called in LivestatusLogUtility::CreateLogCache */
void LogTable::UpdateLogEntries(const Dictionary::Ptr& log_entry_attrs, int line_count, int lineno, const AddRowFunction& addRowFn)
{
	/* additional attributes only for log table */
	log_entry_attrs->Set("lineno", lineno);

	addRowFn(log_entry_attrs, LivestatusGroupByNone, Empty);
}

Object::Ptr LogTable::HostAccessor(const Value& row, const Column::ObjectAccessor&)
{
	String host_name = static_cast<Dictionary::Ptr>(row)->Get("host_name");

	if (host_name.IsEmpty())
		return nullptr;

	return Host::GetByName(host_name);
}

Object::Ptr LogTable::ServiceAccessor(const Value& row, const Column::ObjectAccessor&)
{
	String host_name = static_cast<Dictionary::Ptr>(row)->Get("host_name");
	String service_description = static_cast<Dictionary::Ptr>(row)->Get("service_description");

	if (service_description.IsEmpty() || host_name.IsEmpty())
		return nullptr;

	return Service::GetByNamePair(host_name, service_description);
}

Object::Ptr LogTable::ContactAccessor(const Value& row, const Column::ObjectAccessor&)
{
	String contact_name = static_cast<Dictionary::Ptr>(row)->Get("contact_name");

	if (contact_name.IsEmpty())
		return nullptr;

	return User::GetByName(contact_name);
}

Object::Ptr LogTable::CommandAccessor(const Value& row, const Column::ObjectAccessor&)
{
	String command_name = static_cast<Dictionary::Ptr>(row)->Get("command_name");

	if (command_name.IsEmpty())
		return nullptr;

	CheckCommand::Ptr check_command = CheckCommand::GetByName(command_name);
	if (!check_command) {
		EventCommand::Ptr event_command = EventCommand::GetByName(command_name);
		if (!event_command) {
			NotificationCommand::Ptr notification_command = NotificationCommand::GetByName(command_name);
			if (!notification_command)
				return nullptr;
			else
				return notification_command;
		} else
			return event_command;
	} else
		return check_command;
}

Value LogTable::TimeAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("time");
}

Value LogTable::LinenoAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("lineno");
}

Value LogTable::ClassAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("class");
}

Value LogTable::MessageAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("message");
}

Value LogTable::TypeAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("type");
}

Value LogTable::OptionsAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("options");
}

Value LogTable::CommentAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("comment");
}

Value LogTable::PluginOutputAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("plugin_output");
}

Value LogTable::StateAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("state");
}

Value LogTable::StateTypeAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("state_type");
}

Value LogTable::AttemptAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("attempt");
}

Value LogTable::ServiceDescriptionAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("service_description");
}

Value LogTable::HostNameAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("host_name");
}

Value LogTable::ContactNameAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("contact_name");
}

Value LogTable::CommandNameAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("command_name");
}
