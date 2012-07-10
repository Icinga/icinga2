#ifndef CONSOLELOGGER_H
#define CONSOLELOGGER_H

namespace icinga
{

class ConsoleLogger : public Logger
{
public:
	ConsoleLogger(LogSeverity minSeverity);

protected:
	virtual void ProcessLogEntry(const LogEntry& entry);
};

}

#endif /* CONSOLELOGGER_H */
