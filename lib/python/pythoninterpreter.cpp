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

PythonInterpreter::PythonInterpreter(const PythonLanguage::Ptr& language,
    const Script::Ptr& script)
	: ScriptInterpreter(script), m_Language(language), m_ThreadState(NULL)
{
	PyEval_AcquireLock();

	PyInterpreterState *interp = m_Language->GetMainThreadState()->interp;
	m_ThreadState = PyThreadState_New(interp);

	(void) PyThreadState_Swap(m_ThreadState);
	PyRun_SimpleString(script->GetCode().CStr());
	(void) PyThreadState_Swap(NULL);

	PyEval_ReleaseLock();

	SubscribeFunction("python::Test");
}

PythonInterpreter::~PythonInterpreter(void)
{
	PyEval_AcquireLock();

	(void) PyThreadState_Swap(NULL);

	PyThreadState_Clear(m_ThreadState);
	PyThreadState_Delete(m_ThreadState);

	PyEval_ReleaseLock();
}

void PythonInterpreter::RegisterFunction(const String& name, PyObject *function)
{
	SubscribeFunction(name);

	Py_INCREF(function);
	m_Functions[name] = function;
}

void PythonInterpreter::UnregisterFunction(const String& name)
{
	UnsubscribeFunction(name);

	m_Functions.erase(name);
}

void PythonInterpreter::ProcessCall(const String& function, const ScriptTask::Ptr& task,
    const vector<Value>& arguments)
{
	PyEval_AcquireThread(m_ThreadState);
	std::cout << "Received call for method '" << function << "'" << std::endl;
	PyEval_ReleaseThread(m_ThreadState);

	task->FinishResult(0);
}
