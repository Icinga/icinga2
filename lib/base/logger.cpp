/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/logger.hpp"
#include "base/logger-ti.cpp"
#include "base/application.hpp"
#include "base/streamlogger.hpp"
#include "base/configtype.hpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include "base/context.hpp"
#include "base/scriptglobal.hpp"
#ifdef _WIN32
#include "base/windowseventloglogger.hpp"
#endif /* _WIN32 */
#include <iostream>
#include <utility>

using namespace icinga;

template Log& Log::operator<<(const Value&);
template Log& Log::operator<<(const String&);
template Log& Log::operator<<(const std::string&);
template Log& Log::operator<<(const bool&);
template Log& Log::operator<<(const unsigned int&);
template Log& Log::operator<<(const int&);
template Log& Log::operator<<(const unsigned long&);
template Log& Log::operator<<(const long&);
template Log& Log::operator<<(const double&);

REGISTER_TYPE(Logger);

std::set<Logger::Ptr> Logger::m_Loggers;
std::mutex Logger::m_Mutex;
bool Logger::m_ConsoleLogEnabled = true;
std::atomic<bool> Logger::m_EarlyLoggingEnabled (true);
bool Logger::m_TimestampEnabled = true;
LogSeverity Logger::m_ConsoleLogSeverity = LogInformation;

INITIALIZE_ONCE([]() {
	ScriptGlobal::Set("System.LogDebug", LogDebug, true);
	ScriptGlobal::Set("System.LogNotice", LogNotice, true);
	ScriptGlobal::Set("System.LogInformation", LogInformation, true);
	ScriptGlobal::Set("System.LogWarning", LogWarning, true);
	ScriptGlobal::Set("System.LogCritical", LogCritical, true);
});

/**
 * Constructor for the Logger class.
 */
void Logger::Start(bool runtimeCreated)
{
	ObjectImpl<Logger>::Start(runtimeCreated);

	std::unique_lock<std::mutex> lock(m_Mutex);
	m_Loggers.insert(this);
}

void Logger::Stop(bool runtimeRemoved)
{
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_Loggers.erase(this);
	}

	ObjectImpl<Logger>::Stop(runtimeRemoved);
}

std::set<Logger::Ptr> Logger::GetLoggers()
{
	std::unique_lock<std::mutex> lock(m_Mutex);
	return m_Loggers;
}

/**
 * Retrieves the minimum severity for this logger.
 *
 * @returns The minimum severity.
 */
LogSeverity Logger::GetMinSeverity() const
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
	else
		BOOST_THROW_EXCEPTION(std::invalid_argument("Invalid severity: " + severity));
}

void Logger::DisableConsoleLog()
{
	m_ConsoleLogEnabled = false;
}

void Logger::EnableConsoleLog()
{
	m_ConsoleLogEnabled = true;
}

bool Logger::IsConsoleLogEnabled()
{
	return m_ConsoleLogEnabled;
}

void Logger::SetConsoleLogSeverity(LogSeverity logSeverity)
{
	m_ConsoleLogSeverity = logSeverity;
}

LogSeverity Logger::GetConsoleLogSeverity()
{
	return m_ConsoleLogSeverity;
}

void Logger::DisableEarlyLogging() {
	m_EarlyLoggingEnabled = false;
}

bool Logger::IsEarlyLoggingEnabled() {
	return m_EarlyLoggingEnabled;
}

void Logger::DisableTimestamp()
{
	m_TimestampEnabled = false;
}

void Logger::EnableTimestamp()
{
	m_TimestampEnabled = true;
}

bool Logger::IsTimestampEnabled()
{
	return m_TimestampEnabled;
}

void Logger::ValidateSeverity(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<Logger>::ValidateSeverity(lvalue, utils);

	try {
		StringToSeverity(lvalue());
	} catch (...) {
		BOOST_THROW_EXCEPTION(ValidationError(this, { "severity" }, "Invalid severity specified: " + lvalue()));
	}
}

Log::Log(LogSeverity severity, String facility, const String& message)
	: m_Severity(severity), m_Facility(std::move(facility))
{
	m_Buffer << message;
}

Log::Log(LogSeverity severity, String facility)
	: m_Severity(severity), m_Facility(std::move(facility))
{ }

/**
 * Writes the message to the application's log.
 */
Log::~Log()
{
	LogEntry entry;
	entry.Timestamp = Utility::GetTime();
	entry.Severity = m_Severity;
	entry.Facility = m_Facility;

	{
		auto msg (m_Buffer.str());
		msg.erase(msg.find_last_not_of("\n") + 1u);

		entry.Message = std::move(msg);
	}

	if (m_Severity >= LogWarning) {
		ContextTrace context;

		if (context.GetLength() > 0) {
			std::ostringstream trace;
			trace << context;
			entry.Message += "\nContext:" + trace.str();
		}
	}

	for (const Logger::Ptr& logger : Logger::GetLoggers()) {
		ObjectLock llock(logger);

		if (!logger->IsActive())
			continue;

		if (entry.Severity >= logger->GetMinSeverity())
			logger->ProcessLogEntry(entry);

#ifdef I2_DEBUG /* I2_DEBUG */
		/* Always flush, don't depend on the timer. Enable this for development sprints on Linux/macOS only. Windows crashes. */
		//logger->Flush();
#endif /* I2_DEBUG */
	}

	if (Logger::IsConsoleLogEnabled() && entry.Severity >= Logger::GetConsoleLogSeverity()) {
		StreamLogger::ProcessLogEntry(std::cout, entry);

		/* "Console" might be a pipe/socket (systemd, daemontools, docker, ...),
		 * then cout will not flush lines automatically. */
		std::cout << std::flush;
	}

#ifdef _WIN32
	if (Logger::IsEarlyLoggingEnabled() && entry.Severity >= LogCritical) {
		WindowsEventLogLogger::WriteToWindowsEventLog(entry);
	}
#endif /* _WIN32 */
}

Log& Log::operator<<(const char *val)
{
	m_Buffer << val;
	return *this;
}
