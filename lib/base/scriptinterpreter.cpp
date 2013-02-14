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

#include "i2-base.h"

using namespace icinga;

ScriptInterpreter::ScriptInterpreter(const Script::Ptr& script)
	: m_Thread(&ScriptInterpreter::ThreadWorkerProc, this, script)
{
	m_Thread.detach();
}

void ScriptInterpreter::ThreadWorkerProc(const Script::Ptr& script)
{
	ScriptCall call;

	while (WaitForCall(&call))
		ProcessCall(call);
}

void ScriptInterpreter::EnqueueCall(const ScriptCall& call)
{
	boost::mutex::scoped_lock lock(m_Mutex);
	m_Calls.push_back(call);
	m_CallAvailable.notify_all();
}

bool ScriptInterpreter::WaitForCall(ScriptCall *call)
{
	boost::mutex::scoped_lock lock(m_Mutex);

	while (m_Calls.empty())
		m_CallAvailable.wait(lock);

	*call = m_Calls.front();
	m_Calls.pop_front();

	return true;
}

