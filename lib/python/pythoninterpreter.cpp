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

#include "i2-python.h"

using namespace icinga;

PythonInterpreter::PythonInterpreter(const PythonLanguage::Ptr& language, const Script::Ptr& script)
	: ScriptInterpreter(script), m_Language(language), m_ThreadState(NULL)
{
	PyEval_AcquireLock();

	PyInterpreterState *interp = m_Language->GetMainThreadState()->interp;
	m_ThreadState = PyThreadState_New(interp);

	(void) PyThreadState_Swap(m_ThreadState);
	PyRun_SimpleString(script->GetCode().CStr());
	(void) PyThreadState_Swap(m_Language->GetMainThreadState());

	PyEval_ReleaseLock();
}

PythonInterpreter::~PythonInterpreter(void)
{
	PyEval_AcquireLock();

	PyThreadState_Clear(m_ThreadState);
	PyThreadState_Delete(m_ThreadState);

	PyEval_ReleaseLock();
}

void PythonInterpreter::ProcessCall(const ScriptCall& call)
{
	PyEval_AcquireThread(m_ThreadState);
	PyRun_SimpleString("import antigravity");
	PyEval_ReleaseThread(m_ThreadState);

	call.Task->FinishResult(0);
}
