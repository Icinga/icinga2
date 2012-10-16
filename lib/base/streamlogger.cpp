/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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

#include "i2-base.h"

using namespace icinga;

/**
 * Constructor for the StreamLogger class.
 */
StreamLogger::StreamLogger(void)
	: ILogger(), m_Stream(NULL), m_OwnsStream(false)
{ }

/**
 * Constructor for the StreamLogger class.
 *
 * @param stream The stream.
 */
StreamLogger::StreamLogger(ostream *stream)
	: ILogger(), m_Stream(stream), m_OwnsStream(false)
{ }

/**
 * Destructor for the StreamLogger class.
 */
StreamLogger::~StreamLogger(void)
{
	if (m_OwnsStream)
		delete m_Stream;
}

void StreamLogger::OpenFile(const String& filename)
{
	ofstream *stream = new ofstream();

	try {
		stream->open(filename.CStr(), ofstream::out | ofstream::trunc);

		if (!stream->good())
			throw_exception(runtime_error("Could not open logfile '" + filename + "'"));
	} catch (...) {
		delete stream;
		throw;
	}

	m_Stream = stream;
	m_OwnsStream = true;
}

/**
 * Processes a log entry and outputs it to a stream.
 *
 * @param stream The output stream.
 * @param entry The log entry.
 */
void StreamLogger::ProcessLogEntry(std::ostream& stream, const LogEntry& entry)
{
	char timestamp[100];

	time_t ts = entry.Timestamp;
	tm tmnow = *localtime(&ts);

	strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S %z", &tmnow);

	stream << "[" << timestamp << "] "
		 << Logger::SeverityToString(entry.Severity) << "/" << entry.Facility << ": "
		 << entry.Message << std::endl;
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

