/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#ifdef _WIN32
#include "base/windowseventloglogger.hpp"
#include "base/windowseventloglogger-ti.cpp"
#include "base/windowseventloglogger-provider.h"
#include "base/configtype.hpp"
#include "base/statsfunction.hpp"
#include <windows.h>

using namespace icinga;

REGISTER_TYPE(WindowsEventLogLogger);

REGISTER_STATSFUNCTION(WindowsEventLogLogger, &WindowsEventLogLogger::StatsFunc);

INITIALIZE_ONCE(&WindowsEventLogLogger::StaticInitialize);

static HANDLE l_EventLog = nullptr;

void WindowsEventLogLogger::StaticInitialize()
{
	l_EventLog = RegisterEventSourceA(nullptr, "Icinga 2");
}

void WindowsEventLogLogger::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr&)
{
	DictionaryData nodes;

	for (const WindowsEventLogLogger::Ptr& logger : ConfigType::GetObjectsByType<WindowsEventLogLogger>()) {
		nodes.emplace_back(logger->GetName(), 1);
	}

	status->Set("windowseventloglogger", new Dictionary(std::move(nodes)));
}

/**
 * Processes a log entry and outputs it to the Windows Event Log.
 *
 * @param entry The log entry.
 */
void WindowsEventLogLogger::ProcessLogEntry(const LogEntry& entry)
{
	if (l_EventLog != nullptr) {
		std::string message = Logger::SeverityToString(entry.Severity) + "/" + entry.Facility + ": " + entry.Message;
		std::array<const char *, 1> strings{
			message.c_str()
		};

		WORD eventType;
		switch (entry.Severity) {
			case LogCritical:
				eventType = EVENTLOG_ERROR_TYPE;
				break;
			case LogWarning:
				eventType = EVENTLOG_WARNING_TYPE;
				break;
			default:
				eventType = EVENTLOG_INFORMATION_TYPE;
		}

		ReportEventA(l_EventLog, eventType, 0, MSG_PLAIN_LOG_ENTRY, NULL, strings.size(), 0, strings.data(), NULL);
	}
}

void WindowsEventLogLogger::Flush()
{
	/* Nothing to do here. */
}

#endif /* _WIN32 */
