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

#ifndef PROCESS_H
#define PROCESS_H

#include <boost/thread/mutex.hpp>
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/once.hpp>

namespace icinga
{

/**
 * The result of a Process task.
 *
 * @ingroup base
 */
struct ProcessResult
{
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
class I2_BASE_API Process : public AsyncTask<Process, ProcessResult>
{
public:
	typedef shared_ptr<Process> Ptr;
	typedef weak_ptr<Process> WeakPtr;

	static const deque<Process::Ptr>::size_type MaxTasksPerThread = 512;

	Process(const vector<String>& arguments, const Dictionary::Ptr& extraEnvironment = Dictionary::Ptr());

	static vector<String> SplitCommand(const Value& command);
private:
	vector<String> m_Arguments;
	Dictionary::Ptr m_ExtraEnvironment;

#ifndef _WIN32
	pid_t m_Pid;
	int m_FD;
#endif /* _WIN32 */

	stringstream m_OutputStream;

	ProcessResult m_Result;

	virtual void Run(void);

	static boost::mutex m_Mutex;
	static deque<Process::Ptr> m_Tasks;
#ifndef _WIN32
	static boost::condition_variable m_CV;
	static int m_TaskFd;

	static Timer::Ptr m_StatusTimer;
#endif /* _WIN32 */

	void QueueTask(void);

	void SpawnTask(void);

#ifdef _WIN32
	static void WorkerThreadProc(void);
#else /* _WIN32 */
	static void WorkerThreadProc(int taskFd);

	static void StatusTimerHandler(void);
#endif /* _WIN32 */

	void InitTask(void);
	bool RunTask(void);

	static boost::once_flag m_ThreadOnce;
	static void Initialize(void);
};

}

#endif /* PROCESS_H */
