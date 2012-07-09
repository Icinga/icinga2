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

#ifndef NAGIOSCHECKTASK_H
#define NAGIOSCHECKTASK_H

namespace icinga
{

class I2_CIB_API NagiosCheckTask : public CheckTask
{
public:
	typedef shared_ptr<NagiosCheckTask> Ptr;
	typedef weak_ptr<NagiosCheckTask> WeakPtr;

	static const int MaxChecksPerThread = 128;

	NagiosCheckTask(const Service& service);

	virtual void Enqueue(void);

	static CheckTask::Ptr CreateTask(const Service& service);
	static void FlushQueue(void);

	static void Register(void);

private:
	string m_Command;

	FILE *m_FP;
	stringstream m_OutputStream;
	bool m_UsePopen;
#ifndef _MSC_VER
	void *m_PCloseArg;
#endif /* _MSC_VER */

	static boost::mutex m_Mutex;
	static deque<NagiosCheckTask::Ptr> m_Tasks;
	static vector<NagiosCheckTask::Ptr> m_PendingTasks;
	static condition_variable m_TasksCV;

	static void CheckThreadProc(void);

	bool InitTask(void);
	void ProcessCheckOutput(const string& output);
	bool RunTask(void);
	int GetFD(void) const;
};

}

#endif /* NAGIOSCHECKTASK_H */
