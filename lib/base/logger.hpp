/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef LOGGER_H
#define LOGGER_H

#include "base/i2-base.hpp"
#include "base/logger-ti.hpp"
#include <set>
#include <iosfwd>

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
	static void DisableTimestamp();
	static void EnableTimestamp();
	static bool IsTimestampEnabled();

	static void SetConsoleLogSeverity(LogSeverity logSeverity);
	static LogSeverity GetConsoleLogSeverity();

	void ValidateSeverity(const Lazy<String>& lvalue, const ValidationUtils& utils) final;

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	static std::mutex m_Mutex;
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
