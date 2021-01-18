/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/streamlogger.hpp"
#include "base/streamlogger-ti.cpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include <iostream>

using namespace icinga;

REGISTER_TYPE(StreamLogger);

boost::mutex StreamLogger::m_Mutex;

void StreamLogger::Stop(bool runtimeRemoved)
{
	ObjectImpl<StreamLogger>::Stop(runtimeRemoved);

	// make sure we flush the log data on shutdown, even if we don't call the destructor
	if (m_Stream)
		m_Stream->flush();
}

/**
 * Destructor for the StreamLogger class.
 */
StreamLogger::~StreamLogger()
{
	if (m_FlushLogTimer)
		m_FlushLogTimer->Stop();

	if (m_Stream && m_OwnsStream)
		delete m_Stream;
}

void StreamLogger::FlushLogTimerHandler()
{
	Flush();
}

void StreamLogger::Flush()
{
	ObjectLock oLock (this);

	if (m_Stream)
		m_Stream->flush();
}

void StreamLogger::BindStream(std::ostream *stream, bool ownsStream)
{
	ObjectLock olock(this);

	if (m_Stream && m_OwnsStream)
		delete m_Stream;

	m_Stream = stream;
	m_OwnsStream = ownsStream;

	if (!m_FlushLogTimer) {
		m_FlushLogTimer = new Timer();
		m_FlushLogTimer->SetInterval(1);
		m_FlushLogTimer->OnTimerExpired.connect([this](const Timer * const&) { FlushLogTimerHandler(); });
		m_FlushLogTimer->Start();
	}
}

/**
 * Processes a log entry and outputs it to a stream.
 *
 * @param stream The output stream.
 * @param entry The log entry.
 */
void StreamLogger::ProcessLogEntry(std::ostream& stream, const LogEntry& entry)
{
	String timestamp = Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", entry.Timestamp);

	boost::mutex::scoped_lock lock(m_Mutex);

	if (Logger::IsTimestampEnabled())
		stream << "[" << timestamp << "] ";

	int color;

	switch (entry.Severity) {
		case LogDebug:
			color = Console_ForegroundCyan;
			break;
		case LogNotice:
			color = Console_ForegroundBlue;
			break;
		case LogInformation:
			color = Console_ForegroundGreen;
			break;
		case LogWarning:
			color = Console_ForegroundYellow | Console_Bold;
			break;
		case LogCritical:
			color = Console_ForegroundRed | Console_Bold;
			break;
		default:
			return;
	}

	stream << ConsoleColorTag(color);
	stream << Logger::SeverityToString(entry.Severity);
	stream << ConsoleColorTag(Console_Normal);
	stream << "/" << entry.Facility << ": " << entry.Message << "\n";
}

/**
 * Processes a log entry and outputs it to a stream.
 *
 * @param entry The log entry.
 */
void StreamLogger::ProcessLogEntry(const LogEntry& entry)
{
	ProcessLogEntry(*m_Stream, entry);
}
