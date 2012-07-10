#ifndef CONSOLELOGGER_H
#define CONSOLELOGGER_H

namespace icinga
{

/**
 * A logger that logs to stdout.
 */
class ConsoleLogger : public Logger
{
public:
	ConsoleLogger(LogSeverity minSeverity);

protected:
	virtual void ProcessLogEntry(const LogEntry& entry);
};

}

#endif /* CONSOLELOGGER_H */
