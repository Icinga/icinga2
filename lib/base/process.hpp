/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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

#ifndef PROCESS_H
#define PROCESS_H

#include "base/i2-base.hpp"
#include "base/dictionary.hpp"
#include <boost/function.hpp>
#include <sstream>
#include <deque>
#include <vector>

namespace icinga
{

/**
 * The result of a Process task.
 *
 * @ingroup base
 */
struct ProcessResult
{
	pid_t PID;
	double ExecutionStart;
	double ExecutionEnd;
	long ExitStatus;
	String Output;
};

/**
 * A process task. Executes an external application and returns the exit
 * code and console output.
 *
 * @ingroup base
 */
class I2_BASE_API Process : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(Process);

#ifdef _WIN32
	typedef String Arguments;
	typedef HANDLE ProcessHandle;
	typedef HANDLE ConsoleHandle;
#else /* _WIN32 */
	typedef std::vector<String> Arguments;
	typedef pid_t ProcessHandle;
	typedef int ConsoleHandle;
#endif /* _WIN32 */

	static const std::deque<Process::Ptr>::size_type MaxTasksPerThread = 512;

	Process(const Arguments& arguments, const Dictionary::Ptr& extraEnvironment = Dictionary::Ptr());
	~Process(void);

	void SetTimeout(double timeout);
	double GetTimeout(void) const;

	void Run(const boost::function<void (const ProcessResult&)>& callback = boost::function<void (const ProcessResult&)>());

	pid_t GetPID(void) const;

	static Arguments PrepareCommand(const Value& command);

	static void StaticInitialize(void);
	static void ThreadInitialize(void);

	static String PrettyPrintArguments(const Arguments& arguments);

private:
	Arguments m_Arguments;
	Dictionary::Ptr m_ExtraEnvironment;

	double m_Timeout;

	ProcessHandle m_Process;
	pid_t m_PID;
	ConsoleHandle m_FD;

#ifdef _WIN32
	bool m_ReadPending;
	bool m_ReadFailed;
	OVERLAPPED m_Overlapped;
	char m_ReadBuffer[1024];
#endif /* _WIN32 */

	std::ostringstream m_OutputStream;
	boost::function<void (const ProcessResult&)> m_Callback;
	ProcessResult m_Result;

	static void IOThreadProc(int tid);
	bool DoEvents(void);
	int GetTID(void) const;
};

}

#endif /* PROCESS_H */
