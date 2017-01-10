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

#ifndef _WIN32
#include "base/sysloglogger.hpp"
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"
#include "base/sysloglogger.tcpp"

using namespace icinga;

REGISTER_TYPE(SyslogLogger);

REGISTER_STATSFUNCTION(SyslogLogger, &SyslogLogger::StatsFunc);

void SyslogLogger::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const SyslogLogger::Ptr& sysloglogger : ConfigType::GetObjectsByType<SyslogLogger>()) {
		nodes->Set(sysloglogger->GetName(), 1); //add more stats
	}

	status->Set("sysloglogger", nodes);
}

/**
 * Processes a log entry and outputs it to syslog.
 *
 * @param entry The log entry.
 */
void SyslogLogger::ProcessLogEntry(const LogEntry& entry)
{
	int severity;
	switch (entry.Severity) {
		case LogDebug:
			severity = LOG_DEBUG;
			break;
		case LogNotice:
			severity = LOG_NOTICE;
			break;
		case LogWarning:
			severity = LOG_WARNING;
			break;
		case LogCritical:
			severity = LOG_CRIT;
			break;
		case LogInformation:
		default:
			severity = LOG_INFO;
			break;
	}

	syslog(severity | LOG_USER, "%s", entry.Message.CStr());
}

void SyslogLogger::Flush(void)
{
	/* Nothing to do here. */
}
#endif /* _WIN32 */
