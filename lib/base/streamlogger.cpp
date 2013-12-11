/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "base/streamlogger.h"
#include "base/utility.h"
#include "base/objectlock.h"
#include <boost/thread/thread.hpp>
#include <fstream>
#include <iostream>

using namespace icinga;

REGISTER_TYPE(StreamLogger);

boost::mutex StreamLogger::m_Mutex;

/**
 * Constructor for the StreamLogger class.
 */
void StreamLogger::Start(void)
{
	Logger::Start();

	m_Stream = NULL;
	m_OwnsStream = false;
	m_Tty = false;
}

/**
 * Destructor for the StreamLogger class.
 */
StreamLogger::~StreamLogger(void)
{
	if (m_OwnsStream)
		delete m_Stream;
}

void StreamLogger::FlushLogTimerHandler(void)
{
	m_Stream->flush();
}

void StreamLogger::BindStream(std::ostream *stream, bool ownsStream)
{
	ObjectLock olock(this);

	m_Stream = stream;
	m_OwnsStream = ownsStream;
	m_Tty = IsTty(*stream);
	
	m_FlushLogTimer = make_shared<Timer>();
	m_FlushLogTimer->SetInterval(1);
	m_FlushLogTimer->OnTimerExpired.connect(boost::bind(&StreamLogger::FlushLogTimerHandler, this));
	m_FlushLogTimer->Start();
}

/**
 * Processes a log entry and outputs it to a stream.
 *
 * @param stream The output stream.
 * @param tty Whether the output stream is a TTY.
 * @param entry The log entry.
 */
void StreamLogger::ProcessLogEntry(std::ostream& stream, bool tty, const LogEntry& entry)
{
	String timestamp = Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", entry.Timestamp);

	boost::mutex::scoped_lock lock(m_Mutex);

	if (tty) {
		switch (entry.Severity) {
			case LogWarning:
				stream << "\x1b[1;33m"; // yellow;
				break;
			case LogCritical:
				stream << "\x1b[1;31m"; // red
				break;
			default:
				break;
		}
	}

	stream << "[" << timestamp << "] <" << Utility::GetThreadName() << "> "
		 << Logger::SeverityToString(entry.Severity) << "/" << entry.Facility << ": "
		 << entry.Message;

	if (tty)
		stream << "\x1b[0m"; // clear colors

	stream << "\n";
}

/**
 * Processes a log entry and outputs it to a stream.
 *
 * @param entry The log entry.
 */
void StreamLogger::ProcessLogEntry(const LogEntry& entry)
{
	ProcessLogEntry(*m_Stream, m_Tty, entry);
}

/**
 * Checks whether the specified stream is a terminal.
 *
 * @param stream The stream.
 * @returns true if the stream is a terminal, false otherwise.
 */
bool StreamLogger::IsTty(std::ostream& stream)
{
#ifndef _WIN32
	/* Eww... */
	if (&stream == &std::cout)
		return isatty(fileno(stdout));

	if (&stream == &std::cerr)
		return isatty(fileno(stderr));
#endif /*_ WIN32 */

	return false;
}
