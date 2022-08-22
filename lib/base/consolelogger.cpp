/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#include "base/consolelogger.hpp"

using namespace icinga;

LazyInit<boost::intrusive_ptr<ConsoleLogger>> ConsoleLogger::m_Instance ([]() {
	return boost::intrusive_ptr<ConsoleLogger>(new ConsoleLogger());
});

ConsoleLogger::ConsoleLogger() : m_IsEnabled(true), m_LogSeverity(LogInformation)
{ }

/**
 * Get the instance of this console logger
 *
 * @return ConsoleLogger
 */
const boost::intrusive_ptr<ConsoleLogger>& ConsoleLogger::GetInstance()
{
	return m_Instance.Get();
}

void ConsoleLogger::Disable()
{
	m_IsEnabled = false;
}

void ConsoleLogger::Enable()
{
	m_IsEnabled = true;
}

bool ConsoleLogger::IsEnabled() const
{
	return m_IsEnabled;
}

void ConsoleLogger::SetLogSeverity(LogSeverity logSeverity)
{
	m_LogSeverity = logSeverity;
}

LogSeverity ConsoleLogger::GetLogSeverity()
{
	return m_LogSeverity;
}

/**
 * This just wraps parent class method and binds the passed stream if not already set
 *
 * @param stream The stream to be used for logging the given log entry
 * @param entry  The actual log entry to be logged
 */
void ConsoleLogger::ProcessLogEntry(std::ostream& stream, const LogEntry& entry)
{
	if (m_Stream != &stream) {
		BindStream(&stream, false);
	}

	StreamLogger::ProcessLogEntry(entry);
}
