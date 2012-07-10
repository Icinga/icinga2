#ifndef SYSLOGLOGGER_H
#define SYSLOGLOGGER_H

namespace icinga
{

/**
 * A logger that logs to syslog.
 */
class SyslogLogger : public Logger
{
public:
	SyslogLogger(LogSeverity minSeverity);

protected:
	virtual void ProcessLogEntry(const LogEntry& entry);
};

}

#endif /* SYSLOGLOGGER_H */
