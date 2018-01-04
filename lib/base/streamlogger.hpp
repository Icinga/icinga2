/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef STREAMLOGGER_H
#define STREAMLOGGER_H

#include "base/i2-base.hpp"
#include "base/streamlogger.thpp"
#include "base/timer.hpp"
#include <ostream>

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

	StreamLogger();

	void Stop(bool runtimeRemoved) override;
	~StreamLogger() override;

	void BindStream(std::ostream *stream, bool ownsStream);

	static void ProcessLogEntry(std::ostream& stream, const LogEntry& entry);

protected:
	void ProcessLogEntry(const LogEntry& entry) final;
	void Flush() final;

private:
	static boost::mutex m_Mutex;
	std::ostream *m_Stream;
	bool m_OwnsStream;

	Timer::Ptr m_FlushLogTimer;

	void FlushLogTimerHandler();
};

}

#endif /* STREAMLOGGER_H */
