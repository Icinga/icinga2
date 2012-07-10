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

#ifndef LOGGER_H
#define LOGGER_H

namespace icinga
{

/**
 * Log severity.
 *
 * @ingroup base
 */
enum LogSeverity
{
	LogDebug,
	LogInformation,
	LogWarning,
	LogCritical
};

/**
 * A lot entry.
 *
 * @ingroup base
 */
struct LogEntry {
	time_t Timestamp;
	LogSeverity Severity;
	string Facility;
	string Message;
};

/**
 * Base class for all loggers.
 */
class I2_BASE_API Logger : public Object
{
public:
	typedef shared_ptr<Logger> Ptr;
	typedef weak_ptr<Logger> WeakPtr;

	Logger(LogSeverity minSeverity = LogDebug);

	static void Write(LogSeverity severity, const string& facility,
	    const string& message);

	static void RegisterLogger(const Logger::Ptr& logger);

protected:
	virtual void ProcessLogEntry(const LogEntry& entry) = 0;

	LogSeverity GetMinSeverity(void) const;

private:
	LogSeverity m_MinSeverity;

	static vector<Logger::Ptr> m_Loggers;

	static void ForwardLogEntry(const LogEntry& entry);
};

}

#endif /* LOGGER_H */
