/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#ifndef CONSOLELOGGER_H
#define CONSOLELOGGER_H

#include "base/i2-base.hpp"
#include "base/lazy-init.hpp"
#include "base/logger.hpp"
#include "base/streamlogger.hpp"
#include <boost/intrusive_ptr.hpp>

namespace icinga {

/**
 * A logger that logs to an ostream.
 *
 * This can be used to log entries to all kind of Consoles. This isn't a real config
 * object, it just extends the stream logger to offload console logging from the Logger
 * base class.
 *
 * @ingroup base
 */
class ConsoleLogger final : public StreamLogger
{
public:
	static const boost::intrusive_ptr<ConsoleLogger>& GetInstance();

	void Disable();
	void Enable();
	bool IsEnabled() const;

	void SetLogSeverity(LogSeverity logSeverity);
	LogSeverity GetLogSeverity();

	void ProcessLogEntry(std::ostream& stream, const LogEntry& entry);

private:
	ConsoleLogger();

	static LazyInit<boost::intrusive_ptr<ConsoleLogger>> m_Instance;

	LogSeverity m_LogSeverity;
	bool m_IsEnabled;
};

}

#endif /* CONSOLELOGGER_H */
