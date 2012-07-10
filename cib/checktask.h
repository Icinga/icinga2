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

#ifndef CHECKTASK_H
#define CHECKTASK_H

namespace icinga
{

struct CheckTaskType;

class I2_CIB_API CheckTask : public Object
{
public:
	typedef shared_ptr<CheckTask> Ptr;
	typedef weak_ptr<CheckTask> WeakPtr;

	typedef function<CheckTask::Ptr(const Service&)> Factory;
	typedef function<void()> QueueFlusher;

	Service& GetService(void);
	CheckResult& GetResult(void);

	virtual void Enqueue(void) = 0;

	static void RegisterType(string type, Factory factory, QueueFlusher qflusher);
	static CheckTask::Ptr CreateTask(const Service& service);
	static void Enqueue(const CheckTask::Ptr& task);
	static void FlushQueue(void);

	static int GetTaskHistogramSlots(void);
	static void FinishTask(const CheckTask::Ptr& task);
	static vector<CheckTask::Ptr> GetFinishedTasks(void);

protected:
	CheckTask(const Service& service);

private:
	Service m_Service;
	CheckResult m_Result;

	static map<string, CheckTaskType> m_Types;

	static vector<CheckTask::Ptr> m_FinishedTasks;
	static mutex m_FinishedTasksMutex;
};

struct CheckTaskType
{
	CheckTask::Factory Factory;
	CheckTask::QueueFlusher QueueFlusher;
};

}

#endif /* CHECKTASK_H */
