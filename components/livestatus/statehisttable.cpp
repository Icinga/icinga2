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

#include "livestatus/statehisttable.h"
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
#include <boost/smart_ptr/make_shared.hpp>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>

using namespace icinga;
using namespace livestatus;

StateHistTable::StateHistTable(const String& compat_log_path, const unsigned long& from, const unsigned long& until)
{
	Log(LogInformation, "livestatus", "Pre-selecting log file from " + Convert::ToString(from) + " until " + Convert::ToString(until));

	/* store from & until for FetchRows */
	m_TimeFrom = from;
	m_TimeUntil = until;

	/* create log file index */
	CreateLogIndex(compat_log_path);

	/* m_LogFileIndex map tells which log files are involved ordered by their start timestamp */
	unsigned long ts;
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
			String log_line = String(line);
			type.Trim();
			options.Trim();
			log_line.Trim();

			Dictionary::Ptr bag = GetStateHistAttributes(type, options);

			/* no attributes available - invalid log line */
			if (!bag) {
				//Log(LogDebug, "livestatus", "Skipping invalid statehist line: '" + log_line + "'.");
				continue;
			}

			String host_name = bag->Get("host_name");
			String service_description = bag->Get("service_description");
			unsigned long state = bag->Get("state");
			int log_class = bag->Get("log_class");
			int log_type = bag->Get("log_type");
			String state_type = bag->Get("state_type"); //SOFT, HARD, STARTED, STOPPED, ...

			Service::Ptr state_hist_service;
			/* host alert == get service check */
			if (service_description.IsEmpty()) {
				Host::Ptr state_host = Host::GetByName(host_name);
				state_hist_service = state_host->GetCheckService();
			} else {
				/* assign service ptr as key */
				state_hist_service = Service::GetByNamePair(host_name, service_description);
			}

			/* invalid log line for state history */
			if (!state_hist_service) {
				//Log(LogDebug, "livestatus", "Skipping invalid statehist line: '" + log_line + "'.");
				continue;
			}

			Array::Ptr state_hist_service_states;
			Dictionary::Ptr state_hist_bag;
			unsigned long query_part = until - from;

			/* insert new service states array with values if not existing */
			if (m_ServicesCache.find(state_hist_service) == m_ServicesCache.end()) {

				/* create new values */
				state_hist_service_states = make_shared<Array>();
				state_hist_bag = make_shared<Dictionary>();

				state_hist_bag->Set("host_name", state_hist_service->GetHost()->GetName());
				state_hist_bag->Set("service_description", state_hist_service->GetShortName());
				state_hist_bag->Set("state", state);
				state_hist_bag->Set("in_downtime", 0);
				state_hist_bag->Set("in_host_downtime", 0);
				state_hist_bag->Set("in_notification_period", 1); // assume "always"
				state_hist_bag->Set("is_flapping", 0);
				state_hist_bag->Set("time", time);
				state_hist_bag->Set("lineno", lineno);
				state_hist_bag->Set("log_output", log_line); /* complete line */
				state_hist_bag->Set("from", time); /* starting at current timestamp */
				state_hist_bag->Set("until", time); /* will be updated later on state change */
				state_hist_bag->Set("query_part", query_part); /* required for _part calculations */

				state_hist_service_states->Add(state_hist_bag);

				Log(LogDebug, "livestatus", "statehist: Adding new service '" + state_hist_service->GetName() + "' to services cache.");
			} else {
				{
					boost::mutex::scoped_lock lock(m_Mutex);
					state_hist_service_states = m_ServicesCache[state_hist_service];
					state_hist_bag = state_hist_service_states->Get(state_hist_service_states->GetLength()-1); /* fetch latest state from history */
				}

				/* state duration */
				bool gone = state_hist_bag->Get("gone"); /* this requires initial state logging and setting it if not found TODO */

				/* determine service notifications notification_period and compare against current timestamp */
				bool in_notification_period = true;
				String notification_period_name;
				BOOST_FOREACH(const Notification::Ptr& notification, state_hist_service->GetNotifications()) {
					TimePeriod::Ptr notification_period = notification->GetNotificationPeriod();

					if (notification_period) {
						if (notification_period->IsInside(static_cast<double>(time)))
							in_notification_period = true;
						else
							in_notification_period = false;

						notification_period_name = notification_period->GetName(); // last one wins
					} else
						in_notification_period = true; // assume "always"
				}

				/* check for state changes, flapping & downtime start/end */
				switch (log_type) {
					case LogStateHistTypeHostAlert:
					case LogStateHistTypeHostInitialState:
					case LogStateHistTypeHostCurrentState:
					case LogStateHistTypeServiceAlert:
					case LogStateHistTypeServiceInitialState:
					case LogStateHistTypeServiceCurrentState:
						if (state != state_hist_bag->Get("state")) {
							/* 1. seal old state_hist_bag */
							state_hist_bag->Set("until", time); /* add until record for duration calculation */

							/* 2. add new state_hist_bag */
							Dictionary::Ptr state_hist_bag_new = make_shared<Dictionary>();

							state_hist_bag_new->Set("host_name", state_hist_bag->Get("host_name"));
							state_hist_bag_new->Set("service_description", state_hist_bag->Get("service_description"));
							state_hist_bag_new->Set("state", state);
							state_hist_bag_new->Set("in_downtime", state_hist_bag->Get("in_downtime")); // keep value from previous state!
							state_hist_bag_new->Set("in_host_downtime", state_hist_bag->Get("in_host_downtime")); // keep value from previous state!
							state_hist_bag_new->Set("in_notification_period", (in_notification_period ? 1 : 0));
							state_hist_bag_new->Set("notification_period", notification_period_name);
							state_hist_bag_new->Set("is_flapping", state_hist_bag->Get("is_flapping")); // keep value from previous state!
							state_hist_bag_new->Set("time", time);
							state_hist_bag_new->Set("lineno", lineno);
							state_hist_bag_new->Set("log_output", log_line); /* complete line */
							state_hist_bag_new->Set("from", time); /* starting at current timestamp */
							state_hist_bag_new->Set("until", time + 1); /* will be updated later */
							state_hist_bag_new->Set("query_part", query_part);

							state_hist_service_states->Add(state_hist_bag_new);

							Log(LogDebug, "livestatus", "statehist: State change detected for service '" +
							    state_hist_service->GetName() + "' in '" + log_line + "'.");
						}
						break;
					case LogStateHistTypeHostFlapping:
					case LogStateHistTypeServiceFlapping:
						if (state_type == "STARTED")
							state_hist_bag->Set("is_flapping", 1);
						else if (state_type == "STOPPED" || state_type == "DISABLED")
							state_hist_bag->Set("is_flapping", 0);
						break;
						break;
					case LogStateHistTypeHostDowntimeAlert:
					case LogStateHistTypeServiceDowntimeAlert:
						if (state_type == "STARTED") {
							state_hist_bag->Set("in_downtime", 1);
							if (log_type == LogStateHistTypeHostDowntimeAlert)
								state_hist_bag->Set("in_host_downtime", 1);
						}
						else if (state_type == "STOPPED" || state_type == "CANCELLED") {
							state_hist_bag->Set("in_downtime", 0);
							if (log_type == LogStateHistTypeHostDowntimeAlert)
								state_hist_bag->Set("in_host_downtime", 0);
						}
						break;
					default:
						//nothing to update
						break;
				}

			}

			{
				boost::mutex::scoped_lock lock(m_Mutex);
				m_ServicesCache[state_hist_service] = state_hist_service_states;
			}

			lineno++;
		}

		fp.close();
	}

	AddColumns(this);
}

void StateHistTable::AddColumns(Table *table, const String& prefix,
    const Column::ObjectAccessor& objectAccessor)
{
	table->AddColumn(prefix + "time", Column(&StateHistTable::TimeAccessor, objectAccessor));
	table->AddColumn(prefix + "lineno", Column(&StateHistTable::LinenoAccessor, objectAccessor));
	table->AddColumn(prefix + "from", Column(&StateHistTable::FromAccessor, objectAccessor));
	table->AddColumn(prefix + "until", Column(&StateHistTable::UntilAccessor, objectAccessor));
	table->AddColumn(prefix + "duration", Column(&StateHistTable::DurationAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_part", Column(&StateHistTable::DurationPartAccessor, objectAccessor));
	table->AddColumn(prefix + "state", Column(&StateHistTable::StateAccessor, objectAccessor));
	table->AddColumn(prefix + "host_down", Column(&StateHistTable::HostDownAccessor, objectAccessor));
	table->AddColumn(prefix + "in_downtime", Column(&StateHistTable::InDowntimeAccessor, objectAccessor));
	table->AddColumn(prefix + "in_host_downtime", Column(&StateHistTable::InHostDowntimeAccessor, objectAccessor));
	table->AddColumn(prefix + "is_flapping", Column(&StateHistTable::IsFlappingAccessor, objectAccessor));
	table->AddColumn(prefix + "in_notification_period", Column(&StateHistTable::InNotificationPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "notification_period", Column(&StateHistTable::NotificationPeriodAccessor, objectAccessor));
	table->AddColumn(prefix + "debug_info", Column(&Table::EmptyStringAccessor, objectAccessor));
	table->AddColumn(prefix + "host_name", Column(&StateHistTable::HostNameAccessor, objectAccessor));
	table->AddColumn(prefix + "service_description", Column(&StateHistTable::ServiceDescriptionAccessor, objectAccessor));
	table->AddColumn(prefix + "log_output", Column(&StateHistTable::LogOutputAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_ok", Column(&StateHistTable::DurationOkAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_part_ok", Column(&StateHistTable::DurationPartOkAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_warning", Column(&StateHistTable::DurationWarningAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_part_warning", Column(&StateHistTable::DurationPartWarningAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_critical", Column(&StateHistTable::DurationCriticalAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_part_critical", Column(&StateHistTable::DurationPartCriticalAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_unknown", Column(&StateHistTable::DurationUnknownAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_part_unknown", Column(&StateHistTable::DurationPartUnknownAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_unmonitored", Column(&StateHistTable::DurationUnmonitoredAccessor, objectAccessor));
	table->AddColumn(prefix + "duration_part_unmonitored", Column(&StateHistTable::DurationPartUnmonitoredAccessor, objectAccessor));

	HostsTable::AddColumns(table, "current_host_", boost::bind(&StateHistTable::HostAccessor, _1, objectAccessor));
	ServicesTable::AddColumns(table, "current_service_", boost::bind(&StateHistTable::ServiceAccessor, _1, objectAccessor));
}

String StateHistTable::GetName(void) const
{
	return "log";
}

void StateHistTable::FetchRows(const AddRowFunction& addRowFn)
{
	Service::Ptr state_hist_service;

	BOOST_FOREACH(boost::tie(state_hist_service, boost::tuples::ignore), m_ServicesCache) {
		BOOST_FOREACH(const Dictionary::Ptr& state_hist_bag, m_ServicesCache[state_hist_service]) {
			/* pass a dictionary from state history array */
			addRowFn(state_hist_bag);
		}
	}
}

Object::Ptr StateHistTable::HostAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor)
{
	String host_name = static_cast<Dictionary::Ptr>(row)->Get("host_name");

	if (host_name.IsEmpty())
		return Object::Ptr();

	return Host::GetByName(host_name);
}

Object::Ptr StateHistTable::ServiceAccessor(const Value& row, const Column::ObjectAccessor& parentObjectAccessor)
{
	String host_name = static_cast<Dictionary::Ptr>(row)->Get("host_name");
	String service_description = static_cast<Dictionary::Ptr>(row)->Get("service_description");

	if (service_description.IsEmpty() || host_name.IsEmpty())
		return Object::Ptr();

	return Service::GetByNamePair(host_name, service_description);
}

Value StateHistTable::TimeAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("time");
}

Value StateHistTable::LinenoAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("lineno");
}

Value StateHistTable::FromAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("from");
}

Value StateHistTable::UntilAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("until");
}

Value StateHistTable::DurationAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	return (state_hist_bag->Get("until") - state_hist_bag->Get("from"));
}

Value StateHistTable::DurationPartAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	return (state_hist_bag->Get("until") - state_hist_bag->Get("from")) / state_hist_bag->Get("query_part");
}

Value StateHistTable::StateAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("state");
}

Value StateHistTable::HostDownAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("host_down");
}

Value StateHistTable::InDowntimeAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("in_downtime");
}

Value StateHistTable::InHostDowntimeAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("in_host_downtime");
}

Value StateHistTable::IsFlappingAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("is_flapping");
}

Value StateHistTable::InNotificationPeriodAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("in_notification_period");
}

Value StateHistTable::NotificationPeriodAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("notification_period");
}

Value StateHistTable::HostNameAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("host_name");
}

Value StateHistTable::ServiceDescriptionAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("service_description");
}

Value StateHistTable::LogOutputAccessor(const Value& row)
{
	return static_cast<Dictionary::Ptr>(row)->Get("log_output");
}

Value StateHistTable::DurationOkAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == StateOK)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from"));

	return 0;
}

Value StateHistTable::DurationPartOkAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == StateOK)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from")) / state_hist_bag->Get("query_part");

	return 0;
}

Value StateHistTable::DurationWarningAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == StateWarning)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from"));

	return 0;
}

Value StateHistTable::DurationPartWarningAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == StateWarning)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from")) / state_hist_bag->Get("query_part");

	return 0;
}

Value StateHistTable::DurationCriticalAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == StateCritical)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from"));

	return 0;
}

Value StateHistTable::DurationPartCriticalAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == StateCritical)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from")) / state_hist_bag->Get("query_part");

	return 0;
}

Value StateHistTable::DurationUnknownAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == StateUnknown)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from"));

	return 0;
}

Value StateHistTable::DurationPartUnknownAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == StateUnknown)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from")) / state_hist_bag->Get("query_part");

	return 0;
}

Value StateHistTable::DurationUnmonitoredAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == -1)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from"));

	return 0;
}

Value StateHistTable::DurationPartUnmonitoredAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == -1)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from")) / state_hist_bag->Get("query_part");

	return 0;
}

void StateHistTable::CreateLogIndex(const String& path)
{
	Utility::Glob(path + "/icinga.log", boost::bind(&StateHistTable::CreateLogIndexFileHandler, _1, boost::ref(m_LogFileIndex)));
	Utility::Glob(path + "/archives/*.log", boost::bind(&StateHistTable::CreateLogIndexFileHandler, _1, boost::ref(m_LogFileIndex)));
}

void StateHistTable::CreateLogIndexFileHandler(const String& path, std::map<unsigned long, String>& index)
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

Dictionary::Ptr StateHistTable::GetStateHistAttributes(const String& type, const String& options)
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

		log_class = LogStateHistClassState;
		log_type = LogStateHistTypeHostInitialState;

		host_name = tokens[0];
		state = Host::StateFromString(tokens[1]);
		state_type = tokens[2];
		attempt = atoi(tokens[3].CStr());
		plugin_output = tokens[4];
	}
	else if (boost::algorithm::contains(type, "CURRENT HOST STATE")) {
		if (tokens.size() < 5)
			return Dictionary::Ptr();

		log_class = LogStateHistClassState;
		log_type = LogStateHistTypeHostCurrentState;

		host_name = tokens[0];
		state = Host::StateFromString(tokens[1]);
		state_type = tokens[2];
		attempt = atoi(tokens[3].CStr());
		plugin_output = tokens[4];
	}
	else if (boost::algorithm::contains(type, "HOST ALERT")) {
		if (tokens.size() < 5)
			return Dictionary::Ptr();

		log_class = LogStateHistClassAlert;
		log_type = LogStateHistTypeHostAlert;

		host_name = tokens[0];
		state = Host::StateFromString(tokens[1]);
		state_type = tokens[2];
		attempt = atoi(tokens[3].CStr());
		plugin_output = tokens[4];
	}
	else if (boost::algorithm::contains(type, "HOST DOWNTIME ALERT")) {
		if (tokens.size() < 3)
			return Dictionary::Ptr();

		log_class = LogStateHistClassAlert;
		log_type = LogStateHistTypeHostDowntimeAlert;

		host_name = tokens[0];
		state_type = tokens[1];
		comment = tokens[2];
	}
	else if (boost::algorithm::contains(type, "HOST FLAPPING ALERT")) {
		if (tokens.size() < 3)
			return Dictionary::Ptr();

		log_class = LogStateHistClassAlert;
		log_type = LogStateHistTypeHostFlapping;

		host_name = tokens[0];
		state_type = tokens[1];
		comment = tokens[2];
	}
	else if (boost::algorithm::contains(type, "INITIAL SERVICE STATE")) {
		if (tokens.size() < 6)
			return Dictionary::Ptr();

		log_class = LogStateHistClassState;
		log_type = LogStateHistTypeServiceInitialState;

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

		log_class = LogStateHistClassState;
		log_type = LogStateHistTypeServiceCurrentState;

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

		log_class = LogStateHistClassAlert;
		log_type = LogStateHistTypeServiceAlert;

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

		log_class = LogStateHistClassAlert;
		log_type = LogStateHistTypeServiceDowntimeAlert;

		host_name = tokens[0];
		service_description = tokens[1];
		state_type = tokens[2];
		comment = tokens[3];
	}
	else if (boost::algorithm::contains(type, "SERVICE FLAPPING ALERT")) {
		if (tokens.size() < 4)
			return Dictionary::Ptr();

		log_class = LogStateHistClassAlert;
		log_type = LogStateHistTypeServiceFlapping;

		host_name = tokens[0];
		service_description = tokens[1];
		state_type = tokens[2];
		comment = tokens[3];
	}
	else if (boost::algorithm::contains(type, "TIMEPERIOD TRANSITION")) {
		if (tokens.size() < 4)
			return Dictionary::Ptr();

		log_class = LogStateHistClassState;
		log_type = LogStateHistTypeTimeperiodTransition;

		host_name = tokens[0];
		service_description = tokens[1];
		state_type = tokens[2];
		comment = tokens[3];
	}
	else
		return Dictionary::Ptr();

	Dictionary::Ptr bag = make_shared<Dictionary>();

	bag->Set("log_class", log_class); /* 0 is the default if not populated */
	bag->Set("log_type", log_type);
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
