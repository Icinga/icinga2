#include "i2-base.h"

using namespace icinga;

/**
 * Constructor for the ConsoleLogger class.
 *
 * @param minSeverity Minimum severity for log messages.
 */
ConsoleLogger::ConsoleLogger(LogSeverity minSeverity)
	: Logger(minSeverity)
{ }

/**
 * Processes a log entry and outputs it to standard out.
 *
 * @param entry The log entry.
 */
void ConsoleLogger::ProcessLogEntry(const LogEntry& entry)
{
	char timestamp[100];

	string severityStr;
	switch (entry.Severity) {
		case LogDebug:
			severityStr = "debug";
			break;
		case LogInformation:
			severityStr = "info";
			break;
		case LogWarning:
			severityStr = "warning";
			break;
		case LogCritical:
			severityStr = "critical";
			break;
		default:
			assert(!"Invalid severity specified.");
	}

	tm tmnow = *localtime(&entry.Timestamp);

	strftime(timestamp, sizeof(timestamp), "%Y/%m/%d %H:%M:%S", &tmnow);

	std::cout << "[" << timestamp << "] "
		 << severityStr << "/" << entry.Facility << ": "
		 << entry.Message << std::endl;
}
