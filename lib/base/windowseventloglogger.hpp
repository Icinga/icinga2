/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#pragma once

#ifdef _WIN32
#include "base/i2-base.hpp"
#include "base/windowseventloglogger-ti.hpp"

namespace icinga
{

/**
 * A logger that logs to the Windows Event Log.
 *
 * @ingroup base
 */
class WindowsEventLogLogger final : public ObjectImpl<WindowsEventLogLogger>
{
public:
	DECLARE_OBJECT(WindowsEventLogLogger);
	DECLARE_OBJECTNAME(WindowsEventLogLogger);

	static void StaticInitialize();
	static void StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata);

	static void WriteToWindowsEventLog(const LogEntry& entry);

protected:
	void ProcessLogEntry(const LogEntry& entry) override;
	void Flush() override;
};

}
#endif /* _WIN32 */
