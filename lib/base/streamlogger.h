#ifndef STREAMLOGGER_H
#define STREAMLOGGER_H

namespace icinga
{

/**
 * A logger that logs to an iostream.
 *
 * @ingroup base
 */
class I2_BASE_API StreamLogger : public ILogger
{
public:
	typedef shared_ptr<StreamLogger> Ptr;
	typedef weak_ptr<StreamLogger> WeakPtr;

	StreamLogger(void);
	StreamLogger(std::ostream *stream);
	~StreamLogger(void);

	void OpenFile(const String& filename);

	static void ProcessLogEntry(std::ostream& stream, const LogEntry& entry);

protected:
	virtual void ProcessLogEntry(const LogEntry& entry);

private:
	ostream *m_Stream;
	bool m_OwnsStream;
};

}

#endif /* STREAMLOGGER_H */
