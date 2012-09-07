#ifndef SYSLOGLOGGER_H
#define SYSLOGLOGGER_H

#ifndef _WIN32
namespace icinga
{

/**
 * A logger that logs to syslog.
 */
class I2_BASE_API SyslogLogger : public ILogger
{
public:
	typedef shared_ptr<SyslogLogger> Ptr;
	typedef weak_ptr<SyslogLogger> WeakPtr;

protected:
	virtual void ProcessLogEntry(const LogEntry& entry);
};

}
#endif /* _WIN32 */

#endif /* SYSLOGLOGGER_H */
