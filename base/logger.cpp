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

#include "i2-base.h"

using namespace icinga;

set<Logger::Ptr> Logger::m_Loggers;

/**
 * Constructor for the logger class.
 *
 * @param minSeverity The minimum severity of log messages that should be sent
 *                    to this logger.
 */
Logger::Logger(LogSeverity minSeverity)
	: m_MinSeverity(minSeverity)
{ }

/**
 * Writes a message to the application's log.
 *
 * @param severity The message severity.
 * @param facility The log facility.
 * @param message The message.
 */
void Logger::Write(LogSeverity severity, const string& facility,
    const string& message)
{
	LogEntry entry;
	time(&entry.Timestamp);
	entry.Severity = severity;
	entry.Facility = facility;
	entry.Message = message;

	Event::Post(boost::bind(&Logger::ForwardLogEntry, entry));
}

/**
 * Registers a new logger.
 *
 * @param logger The logger.
 */
void Logger::RegisterLogger(const Logger::Ptr& logger)
{
	assert(Application::IsMainThread());

	m_Loggers.insert(logger);
}

/**
 * Unregisters a logger.
 *
 * @param logger The logger.
 */
void Logger::UnregisterLogger(const Logger::Ptr& logger)
{
	assert(Application::IsMainThread());

	m_Loggers.erase(logger);
}

/**
 * Retrieves the minimum severity for this logger.
 *
 * @returns The minimum severity.
 */
LogSeverity Logger::GetMinSeverity(void) const
{
	return m_MinSeverity;
}

/**
 * Forwards a log entry to the registered loggers.
 *
 * @param entry The log entry.
 */
void Logger::ForwardLogEntry(const LogEntry& entry)
{
	BOOST_FOREACH(const Logger::Ptr& logger, m_Loggers) {
		if (entry.Severity >= logger->GetMinSeverity())
			logger->ProcessLogEntry(entry);
	}
}

string Logger::SeverityToString(LogSeverity severity)
{
	switch (severity) {
		case LogDebug:
			return "debug";
		case LogInformation:
			return "information";
		case LogWarning:
			return "warning";
		case LogCritical:
			return "critical";
		default:
			throw invalid_argument("Invalid severity.");
	}
}

LogSeverity Logger::StringToSeverity(const string& severity)
{
	if (severity == "debug")
		return LogDebug;
	else if (severity == "information")
		return LogInformation;
	else if (severity == "warning")
		return LogWarning;
	else if (severity == "critical")
		return LogCritical;
	else
		throw invalid_argument("Invalid severity: " + severity);
}
