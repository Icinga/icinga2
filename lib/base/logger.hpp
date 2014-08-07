/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/i2-base.hpp"
#include "base/logger.thpp"
#include "base/logger_fwd.hpp"
#include <set>

namespace icinga
{

/**
 * A log entry.
 *
 * @ingroup base
 */
struct LogEntry {
	double Timestamp; /**< The timestamp when this log entry was created. */
	LogSeverity Severity; /**< The severity of this log entry. */
	String Facility; /**< The facility this log entry belongs to. */
	String Message; /**< The log entry's message. */
};

/**
 * A log provider.
 *
 * @ingroup base
 */
class I2_BASE_API Logger : public ObjectImpl<Logger>
{
public:
	DECLARE_PTR_TYPEDEFS(Logger);

	static String SeverityToString(LogSeverity severity);
	static LogSeverity StringToSeverity(const String& severity);

	LogSeverity GetMinSeverity(void) const;

	/**
	 * Processes the log entry and writes it to the log that is
	 * represented by this ILogger object.
	 *
	 * @param entry The log entry that is to be processed.
	 */
	virtual void ProcessLogEntry(const LogEntry& entry) = 0;

	virtual void Flush(void) = 0;

	static std::set<Logger::Ptr> GetLoggers(void);

	static void DisableConsoleLog(void);
	static bool IsConsoleLogEnabled(void);

	static void SetConsoleLogSeverity(LogSeverity logSeverity);
	static LogSeverity GetConsoleLogSeverity(void);

	static void StaticInitialize(void);

protected:
	virtual void Start(void);
	virtual void Stop(void);

private:
	static boost::mutex m_Mutex;
	static std::set<Logger::Ptr> m_Loggers;
	static bool m_ConsoleLogEnabled;
	static LogSeverity m_ConsoleLogSeverity;

	friend I2_BASE_API void Log(LogSeverity severity, const String& facility,
	    const String& message);
};

}

#endif /* LOGGER_H */
