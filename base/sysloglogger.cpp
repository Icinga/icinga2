#include "i2-base.h"

#ifndef _WIN32
using namespace icinga;

/**
 * Processes a log entry and outputs it to syslog.
 *
 * @param entry The log entry.
 */
void SyslogLogger::ProcessLogEntry(const LogEntry& entry)
{
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

	syslog(severity | LOG_USER, "%s", entry.Message.CStr());
}
#endif /* _WIN32 */
