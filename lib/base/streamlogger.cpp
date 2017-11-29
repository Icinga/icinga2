/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/streamlogger.hpp"
#include "base/streamlogger.tcpp"
#include "base/utility.hpp"
#include "base/objectlock.hpp"
#include "base/console.hpp"
#include <iostream>

using namespace icinga;

REGISTER_TYPE(StreamLogger);

boost::mutex StreamLogger::m_Mutex;

/**
 * Constructor for the StreamLogger class.
 */
StreamLogger::StreamLogger(void)
	: m_Stream(NULL), m_OwnsStream(false)
{ }

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
StreamLogger::~StreamLogger(void)
{
	if (m_FlushLogTimer)
		m_FlushLogTimer->Stop();

	if (m_OwnsStream)
		delete m_Stream;
}

void StreamLogger::FlushLogTimerHandler(void)
{
	Flush();
}

void StreamLogger::Flush(void)
{
	if (m_Stream)
		m_Stream->flush();
}

void StreamLogger::BindStream(std::ostream *stream, bool ownsStream)
{
	ObjectLock olock(this);

	if (m_OwnsStream)
		delete m_Stream;

	m_Stream = stream;
	m_OwnsStream = ownsStream;

	m_FlushLogTimer = new Timer();
	m_FlushLogTimer->SetInterval(1);
	m_FlushLogTimer->OnTimerExpired.connect(std::bind(&StreamLogger::FlushLogTimerHandler, this));
	m_FlushLogTimer->Start();
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
