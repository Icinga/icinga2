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

REGISTER_SCRIPTLANGUAGE("Python", PythonLanguage);

PythonLanguage::PythonLanguage(void)
	: ScriptLanguage()
{
	Py_Initialize();
	PyEval_InitThreads();

	Py_SetProgramName(Application::GetArgV()[0]);
	PySys_SetArgv(Application::GetArgC(), Application::GetArgV());

	// See http://docs.python.org/2/c-api/init.html for an explanation.
	PyRun_SimpleString("import sys; sys.path.pop(0)\n");

	m_MainThreadState = PyThreadState_Get();

	m_NativeModule = Py_InitModule("ire", NULL);

	(void) PyThreadState_Swap(NULL);
	PyEval_ReleaseLock();

	String name;
	ScriptFunction::Ptr function;
	BOOST_FOREACH(tie(name, function), ScriptFunction::GetFunctions()) {
		RegisterNativeFunction(name, function);
	}

	ScriptFunction::OnRegistered.connect(boost::bind(&PythonLanguage::RegisterNativeFunction, this, _1, _2));
	ScriptFunction::OnUnregistered.connect(boost::bind(&PythonLanguage::UnregisterNativeFunction, this, _1));
}

PythonLanguage::~PythonLanguage(void)
{
	/* Due to how we're destructing objects it might not be safe to
	 * call Py_Finalize() when the Icinga instance is being shut
	 * down - so don't bother calling it. */
}

ScriptInterpreter::Ptr PythonLanguage::CreateInterpreter(const Script::Ptr& script)
{
	return boost::make_shared<PythonInterpreter>(GetSelf(), script);
}

PyThreadState *PythonLanguage::GetMainThreadState(void) const
{
	return m_MainThreadState;
}

PyObject *PythonLanguage::MarshalToPython(const Value& value)
{
	String svalue;

	switch (value.GetType()) {
		case ValueEmpty:
			Py_INCREF(Py_None);
			return Py_None;

		case ValueNumber:
			return PyFloat_FromDouble(value);

		case ValueString:
			svalue = value;
			return PyString_FromString(svalue.CStr());

		case ValueObject:
			if (value.IsObjectType<DynamicObject>()) {
				DynamicObject::Ptr dobj = value;

				String type = dobj->GetType()->GetName();
				String name = dobj->GetName();

				PyObject *ptype = PyString_FromString(type.CStr());

				if (ptype == NULL)
					return NULL;

				PyObject *pname = PyString_FromString(name.CStr());

				if (pname == NULL) {
					Py_DECREF(ptype);

					return NULL;
				}

				PyObject *result = PyTuple_New(2);

				if (result == NULL) {
					Py_DECREF(ptype);
					Py_DECREF(pname);

					return NULL;
				}

				(void) PyTuple_SetItem(result, 0, ptype);
				(void) PyTuple_SetItem(result, 1, pname);

				return result;
			}

			Py_INCREF(Py_None);
			return Py_None;

		default:
			BOOST_THROW_EXCEPTION(invalid_argument("Unexpected variant type."));
	}
}

Value PythonLanguage::MarshalFromPython(PyObject *value, const ScriptArgumentHint& hint)
{
	if (value == Py_None) {
		return Empty;
	} else if (PyTuple_Check(value)) {
		// TODO: look up object
	} else if (PyFloat_Check(value)) {
		return PyFloat_AsDouble(value);
	} else if (PyString_Check(value)) {
		return PyString_AsString(value);
	} else {
		return Empty;
	}
}

PyObject *PythonLanguage::CallNativeFunction(PyObject *self, PyObject *args)
{
	assert(PyString_Check(self));

	char *name = PyString_AsString(self);

	ScriptFunction::Ptr function = ScriptFunction::GetByName(name);

	vector<Value> arguments;

	if (args != NULL) {
		if (PyTuple_Check(args)) {
			for (Py_ssize_t i = 0; i < PyTuple_Size(args); i++) {
				PyObject *arg = PyTuple_GetItem(args, i);

				arguments.push_back(MarshalFromPython(arg));
			}
		} else {
			arguments.push_back(MarshalFromPython(args));
		}
	}

	ScriptTask::Ptr task = boost::make_shared<ScriptTask>(function, arguments);
	task->Start();
	task->Wait();

	try {
		Value result = task->GetResult();

		return MarshalToPython(result);
	} catch (const std::exception& ex) {
		String message = diagnostic_information(ex);
		PyErr_SetString(PyExc_RuntimeError, message.CStr());

		return NULL;
	}
}

/**
 * Registers a native function.
 *
 * @param name The name of the native function.
 * @param function The function.
 */
void PythonLanguage::RegisterNativeFunction(const String& name, const ScriptFunction::Ptr& function)
{
	(void) PyThreadState_Swap(m_MainThreadState);

	PyObject *pname = PyString_FromString(name.CStr());

	PyMethodDef *md = new PyMethodDef;
	md->ml_name = strdup(name.CStr());
	md->ml_meth = &PythonLanguage::CallNativeFunction;
	md->ml_flags = METH_VARARGS;
	md->ml_doc = NULL;

	PyObject *pfunc = PyCFunction_NewEx(md, pname, m_NativeModule);
	(void) PyModule_AddObject(m_NativeModule, name.CStr(), pfunc);

	(void) PyThreadState_Swap(NULL);
}

/**
 * Unregisters a native function.
 *
 * @param name The name of the native function.
 */
void PythonLanguage::UnregisterNativeFunction(const String& name)
{
	(void) PyThreadState_Swap(m_MainThreadState);

	PyObject *pdict = PyModule_GetDict(m_NativeModule);
	PyObject *pname = PyString_FromString(name.CStr());
	PyCFunctionObject *pfunc = (PyCFunctionObject *)PyDict_GetItem(pdict, pname);

	if (pfunc && PyCFunction_Check(pfunc)) {
		/* Eww. */
		free(const_cast<char *>(pfunc->m_ml->ml_name));
		delete pfunc->m_ml;
	}

	(void) PyDict_DelItem(pdict, pname);
	Py_DECREF(pname);

	(void) PyThreadState_Swap(NULL);
}
