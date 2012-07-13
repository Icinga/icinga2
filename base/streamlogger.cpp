#include "i2-base.h"

using namespace icinga;

/**
 * Constructor for the StreamLogger class.
 *
 * @param minSeverity Minimum severity for log messages.
 */
StreamLogger::StreamLogger(LogSeverity minSeverity)
	: Logger(minSeverity), m_Stream(NULL), m_OwnsStream(false)
{ }

/**
 * Constructor for the StreamLogger class.
 *
 * @param stream The stream.
 * @param minSeverity Minimum severity for log messages.
 */
StreamLogger::StreamLogger(ostream *stream, LogSeverity minSeverity)
	: Logger(minSeverity), m_Stream(stream), m_OwnsStream(false)
{ }

/**
 * Destructor for the StreamLogger class.
 */
StreamLogger::~StreamLogger(void)
{
	if (m_OwnsStream)
		delete m_Stream;
}

void StreamLogger::OpenFile(const string& filename)
{
	ofstream *stream = new ofstream();

	try {
		stream->open(filename.c_str(), ofstream::out | ofstream::trunc);

		if (!stream->good())
			throw runtime_error("Could not open logfile '" + filename + "'");
	} catch (const exception&) {
		delete stream;
		throw;
	}

	m_Stream = stream;
	m_OwnsStream = true;
}

/**
 * Processes a log entry and outputs it to a stream.
 *
 * @param entry The log entry.
 */
void StreamLogger::ProcessLogEntry(const LogEntry& entry)
{
	char timestamp[100];

	tm tmnow = *localtime(&entry.Timestamp);

	strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", &tmnow);

	*m_Stream << "[" << timestamp << "] "
		 << Logger::SeverityToString(entry.Severity) << "/" << entry.Facility << ": "
		 << entry.Message << std::endl;
}
