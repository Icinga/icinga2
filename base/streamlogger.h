#ifndef STREAMLOGGER_H
#define STREAMLOGGER_H

namespace icinga
{

/**
 * A logger that logs to stdout.
 */
class StreamLogger : public Logger
{
public:
	typedef shared_ptr<StreamLogger> Ptr;
	typedef weak_ptr<StreamLogger> WeakPtr;

	StreamLogger(LogSeverity minSeverity);
	StreamLogger(std::ostream *stream, LogSeverity minSeverity);
	~StreamLogger(void);

	void OpenFile(const string& filename);
protected:
	virtual void ProcessLogEntry(const LogEntry& entry);

private:
	ostream *m_Stream;
	bool m_OwnsStream;
};

}

#endif /* STREAMLOGGER_H */
