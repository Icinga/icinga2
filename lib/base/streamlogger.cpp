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

