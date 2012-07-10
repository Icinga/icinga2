#ifndef _WIN32
#include "i2-base.h"

using namespace icinga;

/**
 * Constructor for the SyslogLogger class.
 *
 * @param minSeverity Minimum severity for log messages.
 */
SyslogLogger::SyslogLogger(LogSeverity minSeverity)
	: Logger(minSeverity)
{ }

/**
 * Processes a log entry and outputs it to syslog.
 *
 * @param entry The log entry.
 */
void SyslogLogger::ProcessLogEntry(const LogEntry& entry)
{
	char timestamp[100];

	int severity;
	switch (entry.Severity) {
		case LogDebug:
			severity = LOG_DEBUG;
			break;
		case LogInformation:
			severity = LOG_INFO;
			break;
		case LogWarning:
			severity = LOG_WARNING;
			break;
		case LogCritical:
			severity = LOG_CRIT;
			break;
		default:
			assert(!"Invalid severity specified.");
	}

	syslog(severity | LOG_USER, "%s", entry.Message.c_str());
}
#endif /* _WIN32 */