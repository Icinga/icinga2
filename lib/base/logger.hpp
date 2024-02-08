/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef LOGGER_H
#define LOGGER_H

#include "base/atomic.hpp"
#include "base/i2-base.hpp"
#include "base/logger-ti.hpp"
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
	LogCritical,

	// Just for internal comparision
	LogNothing,
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
	static void DisableEarlyLogging();
	static bool IsEarlyLoggingEnabled();
	static void DisableTimestamp();
	static void EnableTimestamp();
	static bool IsTimestampEnabled();

	static void SetConsoleLogSeverity(LogSeverity logSeverity);
	static LogSeverity GetConsoleLogSeverity();

	static inline
	LogSeverity GetMinLogSeverity()
	{
		return m_MinLogSeverity.load();
	}

	void SetSeverity(const String& value, bool suppress_events = false, const Value& cookie = Empty) override;
	void ValidateSeverity(const Lazy<String>& lvalue, const ValidationUtils& utils) final;

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;

private:
	static void UpdateMinLogSeverity();

	static std::mutex m_Mutex;
	static std::set<Logger::Ptr> m_Loggers;
	static bool m_ConsoleLogEnabled;
	static std::atomic<bool> m_EarlyLoggingEnabled;
	static bool m_TimestampEnabled;
	static LogSeverity m_ConsoleLogSeverity;
	static std::mutex m_UpdateMinLogSeverityMutex;
	static Atomic<LogSeverity> m_MinLogSeverity;
};

class TimeoutLog;

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
	bool m_IsNoOp;

	friend class TimeoutLog;
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

// Logs a message only if a timeout has passed. Useful for logging warnings only if operations take unexpectedly long.
class TimeoutLog : public Log
{
public:
	TimeoutLog(LogSeverity severity, String facility, const String& message)
	: Log(severity, facility, message),
	  m_Start(std::chrono::steady_clock::now())
	{}

	TimeoutLog(LogSeverity severity, String facility)
	: Log(severity, facility),
	  m_Start(std::chrono::steady_clock::now())
	{}

	~TimeoutLog()
	{
		auto duration = std::chrono::steady_clock::now() - m_Start;
		if (duration >= std::chrono::seconds(5)) {
			*this << " (" << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() << "ms)";
		} else {
			m_IsNoOp = true;
		}
	}

private:
	std::chrono::steady_clock::time_point m_Start;
};

}

#endif /* LOGGER_H */
