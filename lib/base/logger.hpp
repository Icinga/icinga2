/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef LOGGER_H
#define LOGGER_H

#include "base/atomic.hpp"
#include "base/i2-base.hpp"
#include "base/configobject.hpp"
#include "base/logger-ti.hpp"
#include <set>
#include <sstream>
#include <vector>

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
	void SetObjectFilter(const Dictionary::Ptr& value, bool suppress_events = false, const Value& cookie = Empty) override;
	void OnAllConfigLoaded() override;

protected:
	void Start(bool runtimeCreated) override;
	void Stop(bool runtimeRemoved) override;
	void ValidateObjectFilter(const Lazy<Dictionary::Ptr>& lvalue, const ValidationUtils& utils) override;

private:
	static void UpdateMinLogSeverity();

	void UpdateCheckObjectFilterCache();

	static std::mutex m_Mutex;
	static std::set<Logger::Ptr> m_Loggers;
	static bool m_ConsoleLogEnabled;
	static std::atomic<bool> m_EarlyLoggingEnabled;
	static bool m_TimestampEnabled;
	static LogSeverity m_ConsoleLogSeverity;
	static std::mutex m_UpdateMinLogSeverityMutex;
	static Atomic<LogSeverity> m_MinLogSeverity;

	Atomic<bool> m_CalledOnAllConfigLoaded {false};
	std::vector<ConfigObject*> m_ObjectFilterCache;
};

class Log
{
public:
	Log() = delete;
	Log(const Log& other) = delete;
	Log& operator=(const Log& rhs) = delete;

	Log(LogSeverity severity, String facility, const String& message = String());
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
	ConfigObject::Ptr m_Involved;
	std::ostringstream m_Buffer;
	bool m_IsNoOp;
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
