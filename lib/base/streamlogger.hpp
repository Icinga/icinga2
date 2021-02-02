/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef STREAMLOGGER_H
#define STREAMLOGGER_H

#include "base/i2-base.hpp"
#include "base/streamlogger-ti.hpp"
#include "base/timer.hpp"
#include <iosfwd>

namespace icinga
{

/**
 * A logger that logs to an iostream.
 *
 * @ingroup base
 */
class StreamLogger : public ObjectImpl<StreamLogger>
{
public:
	DECLARE_OBJECT(StreamLogger);

	void Stop(bool runtimeRemoved) override;
	~StreamLogger() override;

	void BindStream(std::ostream *stream, bool ownsStream);

	static void ProcessLogEntry(std::ostream& stream, const LogEntry& entry);

protected:
	void ProcessLogEntry(const LogEntry& entry) final;
	void Flush() final;

private:
	static std::mutex m_Mutex;
	std::ostream *m_Stream{nullptr};
	bool m_OwnsStream{false};

	Timer::Ptr m_FlushLogTimer;

	void FlushLogTimerHandler();
};

}

#endif /* STREAMLOGGER_H */
