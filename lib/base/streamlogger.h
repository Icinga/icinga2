/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012 Icinga Development Team (http://www.icinga.org/)        *
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
	explicit StreamLogger(ostream *stream);
	~StreamLogger(void);

	void OpenFile(const String& filename);

	static void ProcessLogEntry(ostream& stream, bool tty, const LogEntry& entry);
        static bool IsTty(ostream& stream);

protected:
	virtual void ProcessLogEntry(const LogEntry& entry);

private:
	static boost::mutex m_Mutex;
	ostream *m_Stream;
	bool m_OwnsStream;
        bool m_Tty;
};

}

#endif /* STREAMLOGGER_H */
