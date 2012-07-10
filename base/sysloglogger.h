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
	typedef shared_ptr<SyslogLogger> Ptr;
	typedef weak_ptr<SyslogLogger> WeakPtr;

	SyslogLogger(LogSeverity minSeverity);

protected:
	virtual void ProcessLogEntry(const LogEntry& entry);
};

}

#endif /* SYSLOGLOGGER_H */
