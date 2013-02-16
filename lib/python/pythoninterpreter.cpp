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
	PythonInterpreter *interp = m_Language->GetCurrentInterpreter();
	m_Language->SetCurrentInterpreter(this);

	PyInterpreterState *pinterp = m_Language->GetMainThreadState()->interp;
	m_ThreadState = PyThreadState_New(pinterp);

	(void) PyThreadState_Swap(m_ThreadState);
	PyRun_SimpleString(script->GetCode().CStr());
	(void) PyThreadState_Swap(NULL);

	m_Language->SetCurrentInterpreter(interp);
	PyEval_ReleaseLock();
}

PythonInterpreter::~PythonInterpreter(void)
{
	PyEval_AcquireLock();

	(void) PyThreadState_Swap(NULL);

	PyThreadState_Clear(m_ThreadState);
	PyThreadState_Delete(m_ThreadState);

	PyEval_ReleaseLock();
}

void PythonInterpreter::RegisterPythonFunction(const String& name, PyObject *function)
{
	SubscribeFunction(name);

	Py_INCREF(function);
	m_Functions[name] = function;
}

void PythonInterpreter::UnregisterPythonFunction(const String& name)
{
	UnsubscribeFunction(name);

	m_Functions.erase(name);
}

void PythonInterpreter::ProcessCall(const ScriptTask::Ptr& task, const String& function,
    const vector<Value>& arguments)
{
	PyEval_AcquireThread(m_ThreadState);
	PythonInterpreter *interp = m_Language->GetCurrentInterpreter();
	m_Language->SetCurrentInterpreter(this);

	try {
		map<String, PyObject *>::iterator it = m_Functions.find(function);

		if (it == m_Functions.end())
			BOOST_THROW_EXCEPTION(invalid_argument("Function '" + function + "' does not exist."));

		PyObject *func = it->second;

		PyObject *args = PyTuple_New(arguments.size());

		int i = 0;
		BOOST_FOREACH(const Value& argument, arguments) {
			PyObject *arg = PythonLanguage::MarshalToPython(argument);
			PyTuple_SetItem(args, i, arg);
		}

		PyObject *result = PyObject_CallObject(func, args);

		Py_DECREF(args);

		if (PyErr_Occurred()) {
			PyObject *ptype, *pvalue, *ptraceback;

			PyErr_Fetch(&ptype, &pvalue, &ptraceback);

			String msg = m_Language->ExceptionInfoToString(ptype, pvalue, ptraceback);

			Py_XDECREF(ptype);
			Py_XDECREF(pvalue);
			Py_XDECREF(ptraceback);

			BOOST_THROW_EXCEPTION(runtime_error("Error in Python script:" + msg));
		}

		Value vresult = PythonLanguage::MarshalFromPython(result);
		Py_DECREF(result);

		Application::GetEQ().Post(boost::bind(&ScriptTask::FinishResult, task, vresult));
	} catch (...) {
		Application::GetEQ().Post(boost::bind(&ScriptTask::FinishException, task, boost::current_exception()));
	}

	m_Language->SetCurrentInterpreter(interp);
	PyEval_ReleaseThread(m_ThreadState);
}
