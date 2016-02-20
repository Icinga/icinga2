/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#include "base/logger.hpp"
#include "base/logger.tcpp"
#include "base/application.hpp"
#include "base/streamlogger.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include "base/context.hpp"
#include "base/scriptglobal.hpp"
#include <boost/foreach.hpp>
#include <iostream>

using namespace icinga;

REGISTER_TYPE(Logger);
INITIALIZE_ONCE(&Logger::StaticInitialize);

std::set<Logger::Ptr> Logger::m_Loggers;
boost::mutex Logger::m_Mutex;
bool Logger::m_ConsoleLogEnabled = true;
bool Logger::m_TimestampEnabled = true;
LogSeverity Logger::m_ConsoleLogSeverity = LogInformation;

void Logger::StaticInitialize(void)
{
	ScriptGlobal::Set("LogDebug", LogDebug);
	ScriptGlobal::Set("LogNotice", LogNotice);
	ScriptGlobal::Set("LogInformation", LogInformation);
	ScriptGlobal::Set("LogWarning", LogWarning);
	ScriptGlobal::Set("LogCritical", LogCritical);
}

/**
 * Constructor for the Logger class.
 */
void Logger::Start(bool runtimeCreated)
{
	ObjectImpl<Logger>::Start(runtimeCreated);

	boost::mutex::scoped_lock lock(m_Mutex);
	m_Loggers.insert(this);
}

void Logger::Stop(bool runtimeRemoved)
{
	{
		boost::mutex::scoped_lock lock(m_Mutex);
		m_Loggers.erase(this);
	}

	ObjectImpl<Logger>::Stop(runtimeRemoved);
}

std::set<Logger::Ptr> Logger::GetLoggers(void)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	return m_Loggers;
}

/**
 * Writes a message to the application's log.
 *
 * @param severity The message severity.
 * @param facility The log facility.
 * @param message The message.
 */
void icinga::IcingaLog(LogSeverity severity, const String& facility,
    const String& message)
{
	LogEntry entry;
	entry.Timestamp = Utility::GetTime();
	entry.Severity = severity;
	entry.Facility = facility;
	entry.Message = message;

	if (severity >= LogWarning) {
		ContextTrace context;

		if (context.GetLength() > 0) {
			std::ostringstream trace;
			trace << context;
			entry.Message += "\nContext:" + trace.str();
		}
	}

	BOOST_FOREACH(const Logger::Ptr& logger, Logger::GetLoggers()) {
		ObjectLock llock(logger);

		if (!logger->IsActive())
			continue;

		if (entry.Severity >= logger->GetMinSeverity())
			logger->ProcessLogEntry(entry);
	}

	if (Logger::IsConsoleLogEnabled() && entry.Severity >= Logger::GetConsoleLogSeverity())
		StreamLogger::ProcessLogEntry(std::cout, entry);
}

/**
 * Retrieves the minimum severity for this logger.
 *
 * @returns The minimum severity.
 */
LogSeverity Logger::GetMinSeverity(void) const
{
	String severity = GetSeverity();
	if (severity.IsEmpty())
		return LogInformation;
	else {
		LogSeverity ls = LogInformation;

		try {
			ls = Logger::StringToSeverity(severity);
		} catch (const std::exception&) { /* use the default level */ }

		return ls;
	}
}

/**
 * Converts a severity enum value to a string.
 *
 * @param severity The severity value.
 */
String Logger::SeverityToString(LogSeverity severity)
{
	switch (severity) {
		case LogDebug:
			return "debug";
		case LogNotice:
			return "notice";
		case LogInformation:
			return "information";
		case LogWarning:
			return "warning";
		case LogCritical:
			return "critical";
		default:
			Log(LogCritical, "Logger", "Invalid severity.");
			BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid severity."));
	}
}

/**
 * Converts a string to a severity enum value.
 *
 * @param severity The severity.
 */
LogSeverity Logger::StringToSeverity(const String& severity)
{
	if (severity == "debug")
		return LogDebug;
	else if (severity == "notice")
		return LogNotice;
	else if (severity == "information")
		return LogInformation;
	else if (severity == "warning")
		return LogWarning;
	else if (severity == "critical")
		return LogCritical;
	else {
		Log(LogCritical, "Logger")
		    << "Invalid severity: '" << severity << "'.";
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid severity: " + severity));
	}
}

void Logger::DisableConsoleLog(void)
{
	m_ConsoleLogEnabled = false;
}

void Logger::EnableConsoleLog(void)
{
	m_ConsoleLogEnabled = true;
}

bool Logger::IsConsoleLogEnabled(void)
{
	return m_ConsoleLogEnabled;
}

void Logger::SetConsoleLogSeverity(LogSeverity logSeverity)
{
	m_ConsoleLogSeverity = logSeverity;
}

LogSeverity Logger::GetConsoleLogSeverity(void)
{
	return m_ConsoleLogSeverity;
}

void Logger::DisableTimestamp(bool disable)
{
	m_TimestampEnabled = !disable;
}

bool Logger::IsTimestampEnabled(void)
{
	return m_TimestampEnabled;
}
