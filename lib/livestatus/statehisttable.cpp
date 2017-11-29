/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "livestatus/statehisttable.hpp"
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

StateHistTable::StateHistTable(const String& compat_log_path, time_t from, time_t until)
{
	/* store attributes for FetchRows */
	m_TimeFrom = from;
	m_TimeUntil = until;
	m_CompatLogPath = compat_log_path;

	AddColumns(this);
}

void StateHistTable::UpdateLogEntries(const Dictionary::Ptr& log_entry_attrs, int line_count, int lineno, const AddRowFunction& addRowFn)
{
	unsigned int time = log_entry_attrs->Get("time");
	String host_name = log_entry_attrs->Get("host_name");
	String service_description = log_entry_attrs->Get("service_description");
	unsigned long state = log_entry_attrs->Get("state");
	int log_type = log_entry_attrs->Get("log_type");
	String state_type = log_entry_attrs->Get("state_type"); //SOFT, HARD, STARTED, STOPPED, ...
	String log_line = log_entry_attrs->Get("message"); /* use message from log table */

	Checkable::Ptr checkable;

	if (service_description.IsEmpty())
		checkable = Host::GetByName(host_name);
	else
		checkable = Service::GetByNamePair(host_name, service_description);

	/* invalid log line for state history */
	if (!checkable)
		return;

	Array::Ptr state_hist_service_states;
	Dictionary::Ptr state_hist_bag;
	unsigned long query_part = m_TimeUntil - m_TimeFrom;

	/* insert new service states array with values if not existing */
	if (m_CheckablesCache.find(checkable) == m_CheckablesCache.end()) {

		/* create new values */
		state_hist_service_states = new Array();
		state_hist_bag = new Dictionary();

		Service::Ptr service = dynamic_pointer_cast<Service>(checkable);
		Host::Ptr host;

		if (service)
			host = service->GetHost();
		else
			host = static_pointer_cast<Host>(checkable);

		state_hist_bag->Set("host_name", host->GetName());

		if (service)
			state_hist_bag->Set("service_description", service->GetShortName());

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

		Log(LogDebug, "StateHistTable")
		    << "statehist: Adding new object '" << checkable->GetName() << "' to services cache.";
	} else {
		state_hist_service_states = m_CheckablesCache[checkable];
		state_hist_bag = state_hist_service_states->Get(state_hist_service_states->GetLength()-1); /* fetch latest state from history */

		/* state duration */

		/* determine service notifications notification_period and compare against current timestamp */
		bool in_notification_period = true;
		String notification_period_name;
		for (const Notification::Ptr& notification : checkable->GetNotifications()) {
			TimePeriod::Ptr notification_period = notification->GetPeriod();

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
			case LogEntryTypeHostAlert:
			case LogEntryTypeHostInitialState:
			case LogEntryTypeHostCurrentState:
			case LogEntryTypeServiceAlert:
			case LogEntryTypeServiceInitialState:
			case LogEntryTypeServiceCurrentState:
				if (state != state_hist_bag->Get("state")) {
					/* 1. seal old state_hist_bag */
					state_hist_bag->Set("until", time); /* add until record for duration calculation */

					/* 2. add new state_hist_bag */
					Dictionary::Ptr state_hist_bag_new = new Dictionary();

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

					Log(LogDebug, "StateHistTable")
					    << "statehist: State change detected for object '" << checkable->GetName() << "' in '" << log_line << "'.";
				}
				break;
			case LogEntryTypeHostFlapping:
			case LogEntryTypeServiceFlapping:
				if (state_type == "STARTED")
					state_hist_bag->Set("is_flapping", 1);
				else if (state_type == "STOPPED" || state_type == "DISABLED")
					state_hist_bag->Set("is_flapping", 0);
				break;
				break;
			case LogEntryTypeHostDowntimeAlert:
			case LogEntryTypeServiceDowntimeAlert:
				if (state_type == "STARTED") {
					state_hist_bag->Set("in_downtime", 1);
					if (log_type == LogEntryTypeHostDowntimeAlert)
						state_hist_bag->Set("in_host_downtime", 1);
				}
				else if (state_type == "STOPPED" || state_type == "CANCELLED") {
					state_hist_bag->Set("in_downtime", 0);
					if (log_type == LogEntryTypeHostDowntimeAlert)
						state_hist_bag->Set("in_host_downtime", 0);
				}
				break;
			default:
				//nothing to update
				break;
		}

	}

	m_CheckablesCache[checkable] = state_hist_service_states;

	/* TODO find a way to directly call addRowFn() - right now m_ServicesCache depends on historical lines ("already seen service") */
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

	HostsTable::AddColumns(table, "current_host_", std::bind(&StateHistTable::HostAccessor, _1, objectAccessor));
	ServicesTable::AddColumns(table, "current_service_", std::bind(&StateHistTable::ServiceAccessor, _1, objectAccessor));
}

String StateHistTable::GetName(void) const
{
	return "log";
}

String StateHistTable::GetPrefix(void) const
{
	return "log";
}

void StateHistTable::FetchRows(const AddRowFunction& addRowFn)
{
	Log(LogDebug, "StateHistTable")
	    << "Pre-selecting log file from " << m_TimeFrom << " until " << m_TimeUntil;

	/* create log file index */
	LivestatusLogUtility::CreateLogIndex(m_CompatLogPath, m_LogFileIndex);

	/* generate log cache */
	LivestatusLogUtility::CreateLogCache(m_LogFileIndex, this, m_TimeFrom, m_TimeUntil, addRowFn);

	Checkable::Ptr checkable;

	for (const auto& kv : m_CheckablesCache) {
		for (const Dictionary::Ptr& state_hist_bag : kv.second) {
			/* pass a dictionary from state history array */
			if (!addRowFn(state_hist_bag, LivestatusGroupByNone, Empty))
				return;
		}
	}
}

Object::Ptr StateHistTable::HostAccessor(const Value& row, const Column::ObjectAccessor&)
{
	String host_name = static_cast<Dictionary::Ptr>(row)->Get("host_name");

	if (host_name.IsEmpty())
		return Object::Ptr();

	return Host::GetByName(host_name);
}

Object::Ptr StateHistTable::ServiceAccessor(const Value& row, const Column::ObjectAccessor&)
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

	if (state_hist_bag->Get("state") == ServiceOK)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from"));

	return 0;
}

Value StateHistTable::DurationPartOkAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == ServiceOK)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from")) / state_hist_bag->Get("query_part");

	return 0;
}

Value StateHistTable::DurationWarningAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == ServiceWarning)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from"));

	return 0;
}

Value StateHistTable::DurationPartWarningAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == ServiceWarning)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from")) / state_hist_bag->Get("query_part");

	return 0;
}

Value StateHistTable::DurationCriticalAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == ServiceCritical)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from"));

	return 0;
}

Value StateHistTable::DurationPartCriticalAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == ServiceCritical)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from")) / state_hist_bag->Get("query_part");

	return 0;
}

Value StateHistTable::DurationUnknownAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == ServiceUnknown)
		return (state_hist_bag->Get("until") - state_hist_bag->Get("from"));

	return 0;
}

Value StateHistTable::DurationPartUnknownAccessor(const Value& row)
{
	Dictionary::Ptr state_hist_bag = static_cast<Dictionary::Ptr>(row);

	if (state_hist_bag->Get("state") == ServiceUnknown)
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
