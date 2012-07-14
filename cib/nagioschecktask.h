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

	NagiosCheckTask(const Service& service, const CompletionCallback& completionCallback);

	static CheckTask::Ptr CreateTask(const Service& service, const CompletionCallback& completionCallback);

	static void Register(void);

private:
	string m_Command;
	Process::Ptr m_Process;

	virtual void Run(void);
	void ProcessFinishedHandler(void);
	void ProcessCheckOutput(const string& output);
};

}

#endif /* NAGIOSCHECKTASK_H */
