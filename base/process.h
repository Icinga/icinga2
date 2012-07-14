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

namespace icinga
{

class I2_BASE_API Process : public AsyncTask<Process>
{
public:
	typedef shared_ptr<Process> Ptr;
	typedef weak_ptr<Process> WeakPtr;

	static const int MaxTasksPerThread = 128;

	Process(const string& command, const CompletionCallback& completionCallback);

	time_t GetExecutionStart(void) const;
	time_t GetExecutionEnd(void) const;

	long GetExitStatus(void) const;
	string GetOutput(void) const;

private:
	static bool m_ThreadCreated;

	string m_Command;

	FILE *m_FP;
	stringstream m_OutputStream;
	bool m_UsePopen;
#ifndef _MSC_VER
	void *m_PCloseArg;
#endif /* _MSC_VER */

	time_t m_ExecutionStart;
	time_t m_ExecutionEnd;
	long m_ExitStatus;
	string m_Output;

	virtual void Run(void);

	static boost::mutex m_Mutex;
	static deque<Process::Ptr> m_Tasks;
	static condition_variable m_TasksCV;

	static void WorkerThreadProc(void);

	bool InitTask(void);
	bool RunTask(void);

	int GetFD(void) const;
};

}

#endif /* PROCESS_H */
