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

#include "livestatus/livestatuslogutility.hpp"
#include "icinga/service.hpp"
#include "icinga/host.hpp"
#include "icinga/user.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/notificationcommand.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/logger.hpp"
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <fstream>

using namespace icinga;

void LivestatusLogUtility::CreateLogIndex(const String& path, std::map<time_t, String>& index)
{
	Utility::Glob(path + "/icinga.log", std::bind(&LivestatusLogUtility::CreateLogIndexFileHandler, _1, boost::ref(index)), GlobFile);
	Utility::Glob(path + "/archives/*.log", std::bind(&LivestatusLogUtility::CreateLogIndexFileHandler, _1, boost::ref(index)), GlobFile);
}

void LivestatusLogUtility::CreateLogIndexFileHandler(const String& path, std::map<time_t, String>& index)
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
	time_t ts_start = atoi(buffer+1);

	stream.close();

	Log(LogDebug, "LivestatusLogUtility")
	    << "Indexing log file: '" << path << "' with timestamp start: '" << ts_start << "'.";

	index[ts_start] = path;
}

void LivestatusLogUtility::CreateLogCache(std::map<time_t, String> index, HistoryTable *table,
    time_t from, time_t until, const AddRowFunction& addRowFn)
{
	ASSERT(table);

	/* m_LogFileIndex map tells which log files are involved ordered by their start timestamp */
	unsigned long line_count = 0;
	for (const auto& kv : index) {
		unsigned int ts = kv.first;

		/* skip log files not in range (performance optimization) */
		if (ts < from || ts > until)
			continue;

		String log_file = index[ts];
		int lineno = 0;

		std::ifstream fp;
		fp.exceptions(std::ifstream::badbit);
		fp.open(log_file.CStr(), std::ifstream::in);

		while (fp.good()) {
			std::string line;
			std::getline(fp, line);

			if (line.empty())
				continue; /* Ignore empty lines */

			Dictionary::Ptr log_entry_attrs = LivestatusLogUtility::GetAttributes(line);

			/* no attributes available - invalid log line */
			if (!log_entry_attrs) {
				Log(LogDebug, "LivestatusLogUtility")
				    << "Skipping invalid log line: '" << line << "'.";
				continue;
			}

			table->UpdateLogEntries(log_entry_attrs, line_count, lineno, addRowFn);

			line_count++;
			lineno++;
		}

		fp.close();
	}
}

Dictionary::Ptr LivestatusLogUtility::GetAttributes(const String& text)
{
	Dictionary::Ptr bag = new Dictionary();

	/*
	 * [1379025342] SERVICE NOTIFICATION: contactname;hostname;servicedesc;WARNING;true;foo output
	 */
	unsigned long time = atoi(text.SubStr(1, 11).CStr());

	Log(LogDebug, "LivestatusLogUtility")
	    << "Processing log line: '" << text << "'.";
	bag->Set("time", time);

	size_t colon = text.FindFirstOf(':');
	size_t colon_offset = colon - 13;

	String type = String(text.SubStr(13, colon_offset)).Trim();
	String options = String(text.SubStr(colon + 1)).Trim();

	bag->Set("type", type);
	bag->Set("options", options);

	std::vector<String> tokens;
	boost::algorithm::split(tokens, options, boost::is_any_of(";"));

	/* set default values */
	bag->Set("class", LogEntryClassInfo);
	bag->Set("log_type", 0);
	bag->Set("state", 0);
	bag->Set("attempt", 0);
	bag->Set("message", text); /* used as 'message' in log table, and 'log_output' in statehist table */

	if (type.Contains("INITIAL HOST STATE") ||
	    type.Contains("CURRENT HOST STATE") ||
	    type.Contains("HOST ALERT")) {
		if (tokens.size() < 5)
			return bag;

		bag->Set("host_name", tokens[0]);
		bag->Set("state", Host::StateFromString(tokens[1]));
		bag->Set("state_type", tokens[2]);
		bag->Set("attempt", atoi(tokens[3].CStr()));
		bag->Set("plugin_output", tokens[4]);

		if (type.Contains("INITIAL HOST STATE")) {
			bag->Set("class", LogEntryClassState);
			bag->Set("log_type", LogEntryTypeHostInitialState);
		}
		else if (type.Contains("CURRENT HOST STATE")) {
			bag->Set("class", LogEntryClassState);
			bag->Set("log_type", LogEntryTypeHostCurrentState);
		}
		else {
			bag->Set("class", LogEntryClassAlert);
			bag->Set("log_type", LogEntryTypeHostAlert);
		}

		return bag;
	} else if (type.Contains("HOST DOWNTIME ALERT") ||
		 type.Contains("HOST FLAPPING ALERT")) {
		if (tokens.size() < 3)
			return bag;

		bag->Set("host_name", tokens[0]);
		bag->Set("state_type", tokens[1]);
		bag->Set("comment", tokens[2]);

		if (type.Contains("HOST FLAPPING ALERT")) {
			bag->Set("class", LogEntryClassAlert);
			bag->Set("log_type", LogEntryTypeHostFlapping);
		} else {
			bag->Set("class", LogEntryClassAlert);
			bag->Set("log_type", LogEntryTypeHostDowntimeAlert);
		}

		return bag;
	} else if (type.Contains("INITIAL SERVICE STATE") ||
		 type.Contains("CURRENT SERVICE STATE") ||
		 type.Contains("SERVICE ALERT")) {
		if (tokens.size() < 6)
			return bag;

		bag->Set("host_name", tokens[0]);
		bag->Set("service_description", tokens[1]);
		bag->Set("state", Service::StateFromString(tokens[2]));
		bag->Set("state_type", tokens[3]);
		bag->Set("attempt", atoi(tokens[4].CStr()));
		bag->Set("plugin_output", tokens[5]);

		if (type.Contains("INITIAL SERVICE STATE")) {
			bag->Set("class", LogEntryClassState);
			bag->Set("log_type", LogEntryTypeServiceInitialState);
		}
		else if (type.Contains("CURRENT SERVICE STATE")) {
			bag->Set("class", LogEntryClassState);
			bag->Set("log_type", LogEntryTypeServiceCurrentState);
		}
		else {
			bag->Set("class", LogEntryClassAlert);
			bag->Set("log_type", LogEntryTypeServiceAlert);
		}

		return bag;
	} else if (type.Contains("SERVICE DOWNTIME ALERT") ||
		 type.Contains("SERVICE FLAPPING ALERT")) {
		if (tokens.size() < 4)
			return bag;

		bag->Set("host_name", tokens[0]);
		bag->Set("service_description", tokens[1]);
		bag->Set("state_type", tokens[2]);
		bag->Set("comment", tokens[3]);

		if (type.Contains("SERVICE FLAPPING ALERT")) {
			bag->Set("class", LogEntryClassAlert);
			bag->Set("log_type", LogEntryTypeServiceFlapping);
		} else {
			bag->Set("class", LogEntryClassAlert);
			bag->Set("log_type", LogEntryTypeServiceDowntimeAlert);
		}

		return bag;
	} else if (type.Contains("TIMEPERIOD TRANSITION")) {
		if (tokens.size() < 4)
			return bag;

		bag->Set("class", LogEntryClassState);
		bag->Set("log_type", LogEntryTypeTimeperiodTransition);

		bag->Set("host_name", tokens[0]);
		bag->Set("service_description", tokens[1]);
		bag->Set("state_type", tokens[2]);
		bag->Set("comment", tokens[3]);
	} else if (type.Contains("HOST NOTIFICATION")) {
		if (tokens.size() < 6)
			return bag;

		bag->Set("contact_name", tokens[0]);
		bag->Set("host_name", tokens[1]);
		bag->Set("state_type", tokens[2].CStr());
		bag->Set("state", Service::StateFromString(tokens[3]));
		bag->Set("command_name", tokens[4]);
		bag->Set("plugin_output", tokens[5]);

		bag->Set("class", LogEntryClassNotification);
		bag->Set("log_type", LogEntryTypeHostNotification);

		return bag;
	} else if (type.Contains("SERVICE NOTIFICATION")) {
		if (tokens.size() < 7)
			return bag;

		bag->Set("contact_name", tokens[0]);
		bag->Set("host_name", tokens[1]);
		bag->Set("service_description", tokens[2]);
		bag->Set("state_type", tokens[3].CStr());
		bag->Set("state", Service::StateFromString(tokens[4]));
		bag->Set("command_name", tokens[5]);
		bag->Set("plugin_output", tokens[6]);

		bag->Set("class", LogEntryClassNotification);
		bag->Set("log_type", LogEntryTypeServiceNotification);

		return bag;
	} else if (type.Contains("PASSIVE HOST CHECK")) {
		if (tokens.size() < 3)
			return bag;

		bag->Set("host_name", tokens[0]);
		bag->Set("state", Host::StateFromString(tokens[1]));
		bag->Set("plugin_output", tokens[2]);

		bag->Set("class", LogEntryClassPassive);

		return bag;
	} else if (type.Contains("PASSIVE SERVICE CHECK")) {
		if (tokens.size() < 4)
			return bag;

		bag->Set("host_name", tokens[0]);
		bag->Set("service_description", tokens[1]);
		bag->Set("state", Host::StateFromString(tokens[2]));
		bag->Set("plugin_output", tokens[3]);

		bag->Set("class", LogEntryClassPassive);

		return bag;
	} else if (type.Contains("EXTERNAL COMMAND")) {
		bag->Set("class", LogEntryClassCommand);
		/* string processing not implemented in 1.x */

		return bag;
	} else if (type.Contains("LOG VERSION")) {
		bag->Set("class", LogEntryClassProgram);
		bag->Set("log_type", LogEntryTypeVersion);

		return bag;
	} else if (type.Contains("logging initial states")) {
		bag->Set("class", LogEntryClassProgram);
		bag->Set("log_type", LogEntryTypeInitialStates);

		return bag;
	} else if (type.Contains("starting... (PID=")) {
		bag->Set("class", LogEntryClassProgram);
		bag->Set("log_type", LogEntryTypeProgramStarting);

		return bag;
	}
	/* program */
	else if (type.Contains("restarting...") ||
		 type.Contains("shutting down...") ||
		 type.Contains("Bailing out") ||
		 type.Contains("active mode...") ||
		 type.Contains("standby mode...")) {
		bag->Set("class", LogEntryClassProgram);

		return bag;
	}

	return bag;
}
