#include "i2-base.h"

using namespace icinga;

/**
 * Constructor for the SyslogLogger class.
 *
 * @param minSeverity Minimum severity for log messages.
 */
SyslogLogger::SyslogLogger(const string& ident, LogSeverity minSeverity)
	: Logger(minSeverity)
{
//	openlog(ident.c_str(), 0, LOG_USER);
}

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
