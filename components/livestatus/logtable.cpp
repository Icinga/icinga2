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

#include "livestatus/logtable.h"
#include "livestatus/hoststable.h"
#include "livestatus/servicestable.h"
#include "livestatus/contactstable.h"
#include "livestatus/commandstable.h"
#include "icinga/icingaapplication.h"
#include "icinga/cib.h"
#include "icinga/service.h"
#include "icinga/host.h"
#include "icinga/user.h"
#include "icinga/checkcommand.h"
#include "icinga/eventcommand.h"
#include "icinga/notificationcommand.h"
#include "base/convert.h"
#include "base/utility.h"
#include "base/logger_fwd.h"
#include "base/application.h"
#include "base/objectlock.h"
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>

using namespace icinga;

LogTable::LogTable(const String& compat_log_path, const unsigned long& from, const unsigned long& until)
{
	Log(LogInformation, "livestatus", "Pre-selecting log file from " + Convert::ToString(from) + " until " + Convert::ToString(until));

	/* store from & until for FetchRows */
	m_TimeFrom = from;
	m_TimeUntil = until;

	/* create log file index */
	CreateLogIndex(compat_log_path);

	/* m_LogFileIndex map tells which log files are involved ordered by their start timestamp */
	unsigned long ts;
	unsigned long line_count = 0;
	BOOST_FOREACH(boost::tie(ts, boost::tuples::ignore), m_LogFileIndex) {
		/* skip log files not in range (performance optimization) */
		if (ts < m_TimeFrom || ts > m_TimeUntil)
			continue;

		String log_file = m_LogFileIndex[ts];
		int lineno = 0;

		std::ifstream fp;
		fp.exceptions(std::ifstream::badbit);
		fp.open(log_file.CStr(), std::ifstream::in);

		while (fp.good()) {
			std::string line;
			std::getline(fp, line);

			if (line.empty())
				continue; /* Ignore empty lines */
			/*
			 * [1379025342] SERVICE NOTIFICATION: contactname;hostname;servicedesc;WARNING;true;foo output
			 */
			unsigned long time = atoi(line.substr(1, 11).c_str());

			size_t colon = line.find_first_of(':');
			size_t colon_offset = colon - 13;

			std::string type_str = line.substr(13, colon_offset);
			std::string options_str = line.substr(colon + 1);
			String type = String(type_str);
			String options = String(options_str);
			type.Trim();
			options.Trim();

			Dictionary::Ptr bag = GetLogEntryAttributes(type, options);

			if (!bag)
				continue;

			bag->Set("time", time);
			bag->Set("lineno", lineno);
			bag->Set("message", String(line)); /* complete line */
			bag->Set("type", type);
			bag->Set("options", options);

			{
				boost::mutex::scoped_lock lock(m_Mutex);
				m_RowsCache[line_count] = bag;
			}

			line_count++;
			lineno++;
		}

		fp.close();
	}

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

	HostsTable::AddColumns(table, "current_host_", boost::bind(&LogTable::HostAccessor, _1, objectAccessor));
	ServicesTable::AddColumns(table, "current_service_", boost::bind(&LogTable::ServiceAccessor, _1, objectAccessor));
	ContactsTable::AddColumns(table, "current_contact_", boost::bind(&LogTable::ContactAccessor, _1, objectAccessor));
	CommandsTable::AddColumns(table, "current_command_", boost::bind(&LogTable::CommandAccessor, _1, objectAccessor));
}

String LogTable::GetName(void) const
{
	return "log";
}

void LogTable::FetchRows(const AddRowFunction& addRowFn)
{
	unsigned long line_count;

	BOOST_FOREACH(boost::tie(line_count, boost::tuples::ignore), m_RowsCache) {
		/* pass a dictionary with "line_count" as key */
		addRowFn(m_RowsCache[line_count]);
	}
}

Object::Ptr LogTable::HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor)
{
	String host_name = static_cast<Dictionary::Ptr>(row)->Get("host_name");

	if (host_name.IsEmpty())
		return Object::Ptr();

	return Host::GetByName(host_name);
}

Object::Ptr LogTable::ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor)
{
	String host_name = static_cast<Dictionary::Ptr>(row)->Get("host_name");
	String service_description = static_cast<Dictionary::Ptr>(row)->Get("service_description");

	if (service_description.IsEmpty() || host_name.IsEmpty())
		return Object::Ptr();

	return Service::GetByNamePair(host_name, service_description);
}

Object::Ptr LogTable::ContactAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor)
{
	String contact_name = static_cast<Dictionary::Ptr>(row)->Get("contact_name");

	if (contact_name.IsEmpty())
		return Object::Ptr();

	return User::GetByName(contact_name);
}

Object::Ptr LogTable::CommandAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor)
{
	String command_name = static_cast<Dictionary::Ptr>(row)->Get("command_name");

	if (command_name.IsEmpty())
		return Object::Ptr();

	CheckCommand::Ptr check_command = CheckCommand::GetByName(command_name);
	if (!check_command) {
		EventCommand::Ptr event_command = EventCommand::GetByName(command_name);
		if (!event_command) {
			NotificationCommand::Ptr notification_command = NotificationCommand::GetByName(command_name);
			if (!notification_command)
				return Object::Ptr();
			else
				return notification_command;
		} else
			return event_command;
	} else
		return check_command;

	return Object::Ptr();
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

void LogTable::CreateLogIndex(const String& path)
{
	Utility::Glob(path + "/icinga.log", boost::bind(&LogTable::CreateLogIndexFileHandler, _1, boost::ref(m_LogFileIndex)), GlobFile);
	Utility::Glob(path + "/archives/*.log", boost::bind(&LogTable::CreateLogIndexFileHandler, _1, boost::ref(m_LogFileIndex)), GlobFile);
}

void LogTable::CreateLogIndexFileHandler(const String& path, std::map<unsigned long, String>& index)
{
	std::ifstream stream;
	stream.open(path.CStr(), std::ifstream::in);

	if (!stream)
		BOOST_THROW_EXCEPTION(std::runtime_error("Could not open log file: " + path));

	/* read the first bytes to get the timestamp: [123456789] */
	char buffer[12];

	stream.read(buffer, 12);

	if (buffer[0] != '[' || buffer[11] != ']') {
		/* this can happen for directories too, silently ignore them */
		return;
	}

	/* extract timestamp */
	buffer[11] = 0;
	unsigned int ts_start = atoi(buffer+1);

	stream.close();

	Log(LogDebug, "livestatus", "Indexing log file: '" + path + "' with timestamp start: '" + Convert::ToString(ts_start) + "'.");

	index[ts_start] = path;
}

Dictionary::Ptr LogTable::GetLogEntryAttributes(const String& type, const String& options)
{
	int log_class, log_type = 0;
	unsigned long state, attempt;
	String host_name, service_description, contact_name, command_name, comment, plugin_output, state_type;

	std::vector<String> tokens;
	boost::algorithm::split(tokens, options, boost::is_any_of(";"));

	/* States - TODO refactor */
	if (boost::algorithm::contains(type, "INITIAL HOST STATE")) {
		if (tokens.size() < 5)
			return Dictionary::Ptr();

		log_class = LogClassState;
		log_type = LogTypeHostInitialState;

		host_name = tokens[0];
		state = Host::StateFromString(tokens[1]);
		state_type = tokens[2];
		attempt = atoi(tokens[3].CStr());
		plugin_output = tokens[4];
	}
	else if (boost::algorithm::contains(type, "CURRENT HOST STATE")) {
		if (tokens.size() < 5)
			return Dictionary::Ptr();

		log_class = LogClassState;
		log_type = LogTypeHostCurrentState;

		host_name = tokens[0];
		state = Host::StateFromString(tokens[1]);
		state_type = tokens[2];
		attempt = atoi(tokens[3].CStr());
		plugin_output = tokens[4];
	}
	else if (boost::algorithm::contains(type, "HOST ALERT")) {
		if (tokens.size() < 5)
			return Dictionary::Ptr();

		log_class = LogClassAlert;
		log_type = LogTypeHostAlert;

		host_name = tokens[0];
		state = Host::StateFromString(tokens[1]);
		state_type = tokens[2];
		attempt = atoi(tokens[3].CStr());
		plugin_output = tokens[4];
	}
	else if (boost::algorithm::contains(type, "HOST DOWNTIME ALERT")) {
		if (tokens.size() < 3)
			return Dictionary::Ptr();

		log_class = LogClassAlert;
		log_type = LogTypeHostDowntimeAlert;

		host_name = tokens[0];
		state_type = tokens[1];
		comment = tokens[2];
	}
	else if (boost::algorithm::contains(type, "HOST FLAPPING ALERT")) {
		if (tokens.size() < 3)
			return Dictionary::Ptr();

		log_class = LogClassAlert;
		log_type = LogTypeHostFlapping;

		host_name = tokens[0];
		state_type = tokens[1];
		comment = tokens[2];
	}
	else if (boost::algorithm::contains(type, "INITIAL SERVICE STATE")) {
		if (tokens.size() < 6)
			return Dictionary::Ptr();

		log_class = LogClassState;
		log_type = LogTypeServiceInitialState;

		host_name = tokens[0];
		service_description = tokens[1];
		state = Service::StateFromString(tokens[2]);
		state_type = tokens[3];
		attempt = atoi(tokens[4].CStr());
		plugin_output = tokens[5];
	}
	else if (boost::algorithm::contains(type, "CURRENT SERVICE STATE")) {
		if (tokens.size() < 6)
			return Dictionary::Ptr();

		log_class = LogClassState;
		log_type = LogTypeServiceCurrentState;

		host_name = tokens[0];
		service_description = tokens[1];
		state = Service::StateFromString(tokens[2]);
		state_type = tokens[3];
		attempt = atoi(tokens[4].CStr());
		plugin_output = tokens[5];
	}
	else if (boost::algorithm::contains(type, "SERVICE ALERT")) {
		if (tokens.size() < 6)
			return Dictionary::Ptr();

		log_class = LogClassAlert;
		log_type = LogTypeServiceAlert;

		host_name = tokens[0];
		service_description = tokens[1];
		state = Service::StateFromString(tokens[2]);
		state_type = tokens[3];
		attempt = atoi(tokens[4].CStr());
		plugin_output = tokens[5];
	}
	else if (boost::algorithm::contains(type, "SERVICE DOWNTIME ALERT")) {
		if (tokens.size() < 4)
			return Dictionary::Ptr();

		log_class = LogClassAlert;
		log_type = LogTypeServiceDowntimeAlert;

		host_name = tokens[0];
		service_description = tokens[1];
		state_type = tokens[2];
		comment = tokens[3];
	}
	else if (boost::algorithm::contains(type, "SERVICE FLAPPING ALERT")) {
		if (tokens.size() < 4)
			return Dictionary::Ptr();

		log_class = LogClassAlert;
		log_type = LogTypeServiceFlapping;

		host_name = tokens[0];
		service_description = tokens[1];
		state_type = tokens[2];
		comment = tokens[3];
	}
	else if (boost::algorithm::contains(type, "TIMEPERIOD TRANSITION")) {
		if (tokens.size() < 4)
			return Dictionary::Ptr();

		log_class = LogClassState;
		log_type = LogTypeTimeperiodTransition;

		host_name = tokens[0];
		service_description = tokens[1];
		state_type = tokens[2];
		comment = tokens[3];
	}
	/* Notifications - TODO refactor */
	else if (boost::algorithm::contains(type, "HOST NOTIFICATION")) {
		if (tokens.size() < 6)
			return Dictionary::Ptr();

		log_class = LogClassNotification;
		log_type = LogTypeHostNotification;

		contact_name = tokens[0];
		host_name = tokens[1];
		state_type = tokens[2];
		state = Host::StateFromString(tokens[3]);
		command_name = tokens[4];
		plugin_output = tokens[5];
	}
	else if (boost::algorithm::contains(type, "SERVICE NOTIFICATION")) {
		if (tokens.size() < 7)
			return Dictionary::Ptr();

		log_class = LogClassNotification;
		log_type = LogTypeHostNotification;

		contact_name = tokens[0];
		host_name = tokens[1];
		service_description = tokens[2];
		state_type = tokens[3];
		state = Service::StateFromString(tokens[4]);
		command_name = tokens[5];
		plugin_output = tokens[6];
	}
	/* Passive Checks - TODO refactor */
	else if (boost::algorithm::contains(type, "PASSIVE HOST CHECK")) {
		if (tokens.size() < 3)
			return Dictionary::Ptr();

		log_class = LogClassPassive;

		host_name = tokens[0];
		state = Host::StateFromString(tokens[1]);
		plugin_output = tokens[2];
	}
	else if (boost::algorithm::contains(type, "PASSIVE SERVICE CHECK")) {
		if (tokens.size() < 4)
			return Dictionary::Ptr();

		log_class = LogClassPassive;

		host_name = tokens[0];
		service_description = tokens[1];
		state = Service::StateFromString(tokens[2]);
		plugin_output = tokens[3];
	}
	/* External Command - TODO refactor */
	else if (boost::algorithm::contains(type, "EXTERNAL COMMAND")) {
		log_class = LogClassCommand;
		/* string processing not implemented in 1.x */
	}
	/* normal text entries */
	else if (boost::algorithm::contains(type, "LOG VERSION")) {
		log_class = LogClassProgram;
		log_type = LogTypeVersion;
	}
	else if (boost::algorithm::contains(type, "logging initial states")) {
		log_class = LogClassProgram;
		log_type = LogTypeInitialStates;
	}
	else if (boost::algorithm::contains(type, "starting... (PID=")) {
		log_class = LogClassProgram;
		log_type = LogTypeProgramStarting;
	}
	/* program */
	else if (boost::algorithm::contains(type, "restarting...") ||
		 boost::algorithm::contains(type, "shutting down...") ||
		 boost::algorithm::contains(type, "Bailing out") ||
		 boost::algorithm::contains(type, "active mode...") ||
		 boost::algorithm::contains(type, "standby mode...")) {
		log_class = LogClassProgram;
	} else
		return Dictionary::Ptr();

	Dictionary::Ptr bag = make_shared<Dictionary>();

	bag->Set("class", log_class); /* 0 is the default if not populated */
	bag->Set("comment", comment);
	bag->Set("plugin_output", plugin_output);
	bag->Set("state", state);
	bag->Set("state_type", state_type);
	bag->Set("attempt", attempt);
	bag->Set("host_name", host_name);
	bag->Set("service_description", service_description);
	bag->Set("contact_name", contact_name);
	bag->Set("command_name", command_name);

	return bag;
}
