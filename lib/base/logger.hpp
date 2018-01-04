/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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
#include <set>
#include <sstream>

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
	LogNotice,
	LogInformation,
	LogWarning,
	LogCritical
};

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
class Logger : public ObjectImpl<Logger>
{
public:
	DECLARE_OBJECT(Logger);

	static String SeverityToString(LogSeverity severity);
	static LogSeverity StringToSeverity(const String& severity);

	LogSeverity GetMinSeverity() const;

	/**
	 * Processes the log entry and writes it to the log that is
	 * represented by this ILogger object.
	 *
	 * @param entry The log entry that is to be processed.
	 */
	virtual void ProcessLogEntry(const LogEntry& entry) = 0;

	virtual void Flush() = 0;

	static std::set<Logger::Ptr> GetLoggers();

	static void DisableConsoleLog();
	static void EnableConsoleLog();
	static bool IsConsoleLogEnabled();
	static void DisableTimestamp(bool);
	static bool IsTimestampEnabled();

	static void SetConsoleLogSeverity(LogSeverity logSeverity);
	static LogSeverity GetConsoleLogSeverity();

	void ValidateSeverity(const String& value, const ValidationUtils& utils) final;

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	static boost::mutex m_Mutex;
	static std::set<Logger::Ptr> m_Loggers;
	static bool m_ConsoleLogEnabled;
	static bool m_TimestampEnabled;
	static LogSeverity m_ConsoleLogSeverity;
};

class Log
{
public:
	Log() = delete;
	Log(const Log& other) = delete;
	Log& operator=(const Log& rhs) = delete;

	Log(LogSeverity severity, String facility, const String& message);
	Log(LogSeverity severity, String facility);

	~Log();

	template<typename T>
	Log& operator<<(const T& val)
	{
		m_Buffer << val;
		return *this;
	}

	Log& operator<<(const char *val);

private:
	LogSeverity m_Severity;
	String m_Facility;
	std::ostringstream m_Buffer;
};

extern template Log& Log::operator<<(const Value&);
extern template Log& Log::operator<<(const String&);
extern template Log& Log::operator<<(const std::string&);
extern template Log& Log::operator<<(const bool&);
extern template Log& Log::operator<<(const unsigned int&);
extern template Log& Log::operator<<(const int&);
extern template Log& Log::operator<<(const unsigned long&);
extern template Log& Log::operator<<(const long&);
extern template Log& Log::operator<<(const double&);

}

#endif /* LOGGER_H */
