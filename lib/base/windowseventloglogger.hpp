// SPDX-FileCopyrightText: 2021 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef WINDOWSEVENTLOGLOGGER_H
#define WINDOWSEVENTLOGLOGGER_H

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

#endif /* WINDOWSEVENTLOGLOGGER_H */
