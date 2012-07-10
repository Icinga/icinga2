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

vector<Logger::Ptr> Logger::m_Loggers;

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

	Event::Ptr ev = boost::make_shared<Event>();
	ev->OnEventDelivered.connect(boost::bind(&Logger::ForwardLogEntry, entry));
	Event::Post(ev);
}

void Logger::RegisterLogger(const Logger::Ptr& logger)
{
	m_Loggers.push_back(logger);
}

LogSeverity Logger::GetMinSeverity(void) const
{
	return m_MinSeverity;
}

void Logger::ForwardLogEntry(const LogEntry& entry)
{
	vector<Logger::Ptr>::iterator it;
	for (it = m_Loggers.begin(); it != m_Loggers.end(); it++) {
		Logger::Ptr logger = *it;

		if (entry.Severity >= logger->GetMinSeverity())
			logger->ProcessLogEntry(entry);
	}
}
