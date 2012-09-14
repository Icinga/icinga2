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

REGISTER_CLASS(Logger);

/**
 * Constructor for the logger class.
 *
 * @param minSeverity The minimum severity of log messages that should be sent
 *                    to this logger.
 */
Logger::Logger(const Dictionary::Ptr& properties)
	: DynamicObject(properties)
{
	RegisterAttribute("type", Attribute_Config);
	RegisterAttribute("path", Attribute_Config);
	RegisterAttribute("severity", Attribute_Config);

	if (!IsLocal())
		throw_exception(runtime_error("Logger objects must be local."));

	String type = Get("type");
	if (type.IsEmpty())
		throw_exception(runtime_error("Logger objects must have a 'type' property."));

	ILogger::Ptr impl;

	if (type == "syslog") {
#ifndef _WIN32
		impl = boost::make_shared<SyslogLogger>();
#else /* _WIN32 */
		throw_exception(invalid_argument("Syslog is not supported on Windows."));
#endif /* _WIN32 */
	} else if (type == "file") {
		String path = Get("path");
		if (path.IsEmpty())
			throw_exception(invalid_argument("'log' object of type 'file' must have a 'path' property"));

		StreamLogger::Ptr slogger = boost::make_shared<StreamLogger>();
		slogger->OpenFile(path);

		impl = slogger;
	} else if (type == "console") {
		impl = boost::make_shared<StreamLogger>(&std::cout);
	} else {
		throw_exception(runtime_error("Unknown log type: " + type));
	}

	impl->m_Config = this;
	m_Impl = impl;
}

/**
 * Writes a message to the application's log.
 *
 * @param severity The message severity.
 * @param facility The log facility.
 * @param message The message.
 */
void Logger::Write(LogSeverity severity, const String& facility,
    const String& message)
{
	LogEntry entry;
	entry.Timestamp = Utility::GetTime();
	entry.Severity = severity;
	entry.Facility = facility;
	entry.Message = message;

	Event::Post(boost::bind(&Logger::ForwardLogEntry, entry));
}

/**
 * Retrieves the minimum severity for this logger.
 *
 * @returns The minimum severity.
 */
LogSeverity Logger::GetMinSeverity(void) const
{
	String severity = Get("severity");
	if (severity.IsEmpty())
		return LogInformation;
	else
		return Logger::StringToSeverity(severity);
}

/**
 * Forwards a log entry to the registered loggers.
 *
 * @param entry The log entry.
 */
void Logger::ForwardLogEntry(const LogEntry& entry)
{
	bool processed = false;

	DynamicObject::Ptr object;
	BOOST_FOREACH(tie(tuples::ignore, object), DynamicObject::GetObjects("Logger")) {
		Logger::Ptr logger = dynamic_pointer_cast<Logger>(object);

		if (entry.Severity >= logger->GetMinSeverity())
			logger->m_Impl->ProcessLogEntry(entry);

		processed = true;
	}

	if (!processed && entry.Severity >= LogInformation)
		StreamLogger::ProcessLogEntry(std::cout, entry);
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
		case LogInformation:
			return "information";
		case LogWarning:
			return "warning";
		case LogCritical:
			return "critical";
		default:
			throw_exception(invalid_argument("Invalid severity."));
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
	else if (severity == "information")
		return LogInformation;
	else if (severity == "warning")
		return LogWarning;
	else if (severity == "critical")
		return LogCritical;
	else
		throw_exception(invalid_argument("Invalid severity: " + severity));
}
