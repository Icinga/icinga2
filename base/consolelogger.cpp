#include "i2-base.h"

using namespace icinga;

ConsoleLogger::ConsoleLogger(LogSeverity minSeverity)
	: Logger(minSeverity)
{ }

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
