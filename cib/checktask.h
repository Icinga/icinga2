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

class I2_CIB_API CheckTask : public AsyncTask<CheckTask>
{
public:
	typedef shared_ptr<CheckTask> Ptr;
	typedef weak_ptr<CheckTask> WeakPtr;

	typedef function<CheckTask::Ptr(const Service&)> Factory;

	Service& GetService(void);
	CheckResult& GetResult(void);

	static void RegisterType(string type, Factory factory);
	static CheckTask::Ptr CreateTask(const Service& service);

	static int GetTaskHistogramSlots(void);

protected:
	CheckTask(const Service& service);

	virtual void Run(void) = 0;

private:
	Service m_Service;
	CheckResult m_Result;

	static map<string, CheckTaskType> m_Types;
};

struct CheckTaskType
{
	CheckTask::Factory Factory;
};

}

#endif /* CHECKTASK_H */
