/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2013 Icinga Development Team (http://www.icinga.org/)   *
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

#include "python/pythonlanguage.h"
#include "python/pythoninterpreter.h"
#include "base/scriptfunction.h"
#include "base/dynamictype.h"
#include "base/objectlock.h"
#include "base/application.h"
#include "base/array.h"
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

using namespace icinga;

PythonInterpreter *PythonLanguage::m_CurrentInterpreter;

REGISTER_SCRIPTLANGUAGE("Python", PythonLanguage);

PyMethodDef PythonLanguage::m_NativeMethodDef[] = {
	{ "RegisterFunction", &PythonLanguage::PyRegisterFunction, METH_VARARGS, NULL },
	{ NULL, NULL, 0, NULL } /* sentinel */
};

PythonLanguage::PythonLanguage(void)
	: ScriptLanguage(), m_Initialized(false)
{ }

void PythonLanguage::InitializeOnce(void)
{
	ObjectLock olock(this);

	if (m_Initialized)
		return;

	Py_Initialize();
	PyEval_InitThreads();

	Py_SetProgramName(Application::GetArgV()[0]);
	PySys_SetArgv(Application::GetArgC(), Application::GetArgV());

	// See http://docs.python.org/2/c-api/init.html for an explanation.
	PyRun_SimpleString("import sys; sys.path.pop(0)\n");

	m_MainThreadState = PyThreadState_Get();

	m_TracebackModule = PyImport_ImportModule("traceback");

	m_NativeModule = Py_InitModule("ire", m_NativeMethodDef);

	(void) PyThreadState_Swap(NULL);
	PyEval_ReleaseLock();

	String name;
	BOOST_FOREACH(boost::tie(name, boost::tuples::ignore), ScriptFunctionRegistry::GetInstance()->GetItems()) {
		RegisterNativeFunction(name);
	}

	ScriptFunctionRegistry::GetInstance()->OnRegistered.connect(boost::bind(&PythonLanguage::RegisterNativeFunction, this, _1));
	ScriptFunctionRegistry::GetInstance()->OnUnregistered.connect(boost::bind(&PythonLanguage::UnregisterNativeFunction, this, _1));

	m_Initialized = true;
}

PythonLanguage::~PythonLanguage(void)
{
	/* Due to how we're destructing objects it might not be safe to
	 * call Py_Finalize() when the Icinga instance is being shut
	 * down - so don't bother calling it. */
}

ScriptInterpreter::Ptr PythonLanguage::CreateInterpreter(const Script::Ptr& script)
{
	InitializeOnce();

	return make_shared<PythonInterpreter>(GetSelf(), script);
}

PyThreadState *PythonLanguage::GetMainThreadState(void) const
{
	ObjectLock olock(this);

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
			} else if (value.IsObjectType<Dictionary>()) {
				Dictionary::Ptr dict = value;
				ObjectLock olock(dict);

				PyObject *pdict = PyDict_New();

				String key;
				Value value;
				BOOST_FOREACH(boost::tie(key, value), dict) {
					PyObject *dv = MarshalToPython(value);

					PyDict_SetItemString(pdict, key.CStr(), dv);

					Py_DECREF(dv);
				}

				return pdict;
			} else if (value.IsObjectType<Array>()) {
				Array::Ptr arr = value;
				ObjectLock olock(arr);

				PyObject *plist = PyList_New(0);

				BOOST_FOREACH(const Value& value, arr) {
					PyObject *dv = MarshalToPython(value);

					PyList_Append(plist, dv);

					Py_DECREF(dv);
				}

				return plist;
			}

			Py_INCREF(Py_None);
			return Py_None;

		default:
			BOOST_THROW_EXCEPTION(std::invalid_argument("Unexpected variant type."));
	}
}

Value PythonLanguage::MarshalFromPython(PyObject *value)
{
	if (value == Py_None) {
		return Empty;
	} else if (PyDict_Check(value)) {
		Dictionary::Ptr dict = make_shared<Dictionary>();

		PyObject *dk, *dv;
		Py_ssize_t pos = 0;

		while (PyDict_Next(value, &pos, &dk, &dv)) {
			String ik = PyString_AsString(dk);
			Value iv = MarshalFromPython(dv);

			dict->Set(ik, iv);
		}

		return dict;
	} else if (PyList_Check(value)) {
		Array::Ptr arr = make_shared<Array>();

		for (Py_ssize_t pos = 0; pos < PyList_Size(value); pos++) {
			PyObject *dv = PyList_GetItem(value, pos);
			Value iv = MarshalFromPython(dv);

			arr->Add(iv);
		}

		return arr;
	} else if (PyTuple_Check(value) && PyTuple_Size(value) == 2) {
		PyObject *ptype, *pname;

		ptype = PyTuple_GetItem(value, 0);

		if (ptype == NULL || !PyString_Check(ptype))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Tuple must contain two strings."));

		String type = PyString_AsString(ptype);

		pname = PyTuple_GetItem(value, 1);

		if (pname == NULL || !PyString_Check(pname))
			BOOST_THROW_EXCEPTION(std::invalid_argument("Tuple must contain two strings."));

		String name = PyString_AsString(pname);

		DynamicType::Ptr dtype = DynamicType::GetByName(type);

		if (!dtype)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Type '" + type + "' does not exist."));

		DynamicObject::Ptr object = dtype->GetObject(name);

		if (!object)
			BOOST_THROW_EXCEPTION(std::invalid_argument("Object '" + name + "' of type '" + type + "' does not exist."));

		return object;
	} else if (PyFloat_Check(value)) {
		return PyFloat_AsDouble(value);
	} else if (PyInt_Check(value)) {
		return PyInt_AsLong(value);
	} else if (PyString_Check(value)) {
		return PyString_AsString(value);
	} else {
		return Empty;
	}
}

String PythonLanguage::ExceptionInfoToString(PyObject *type, PyObject *exc, PyObject *tb) const
{
	ObjectLock olock(this);

	PyObject *tb_dict = PyModule_GetDict(m_TracebackModule);
	PyObject *format_exception = PyDict_GetItemString(tb_dict, "format_exception");

	if (!PyCallable_Check(format_exception))
		return "Failed to format exception information.";

	PyObject *result = PyObject_CallFunctionObjArgs(format_exception, type, exc, tb, NULL);

	Py_DECREF(format_exception);
	Py_DECREF(tb_dict);

	if (!result || !PyList_Check(result)) {
		Py_XDECREF(result);

		return "format_exception() returned something that is not a list.";
	}

	String msg;

	for (Py_ssize_t i = 0; i < PyList_Size(result); i++) {
		PyObject *li = PyList_GetItem(result, i);

		if (!li || !PyString_Check(li)) {
			Py_DECREF(result);

			return "format_exception() returned something that is not a list of strings.";
		}

		msg += PyString_AsString(li);
	}

	Py_DECREF(result);

	return msg;
}

PyObject *PythonLanguage::PyCallNativeFunction(PyObject *self, PyObject *args)
{
	assert(PyString_Check(self));

	char *name = PyString_AsString(self);

	ScriptFunction::Ptr function = ScriptFunctionRegistry::GetInstance()->GetItem(name);

	std::vector<Value> arguments;

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

	PyThreadState *tstate = PyEval_SaveThread();

	Value result;

	try {
		result = function->Invoke(arguments);
	} catch (const std::exception& ex) {
		PyEval_RestoreThread(tstate);

		String message = DiagnosticInformation(ex);
		PyErr_SetString(PyExc_RuntimeError, message.CStr());

		return NULL;
	}

	PyEval_RestoreThread(tstate);

	return MarshalToPython(result);
}

/**
 * Registers a native function.
 *
 * @param name The name of the native function.
 */
void PythonLanguage::RegisterNativeFunction(const String& name)
{
	ObjectLock olock(this);

	PyThreadState *tstate = PyThreadState_Swap(m_MainThreadState);

	PyObject *pname = PyString_FromString(name.CStr());

	PyMethodDef *md = new PyMethodDef;
	md->ml_name = strdup(name.CStr());
	md->ml_meth = &PythonLanguage::PyCallNativeFunction;
	md->ml_flags = METH_VARARGS;
	md->ml_doc = NULL;

	PyObject *pfunc = PyCFunction_NewEx(md, pname, m_NativeModule);
	(void) PyModule_AddObject(m_NativeModule, name.CStr(), pfunc);

	(void) PyThreadState_Swap(tstate);
}

/**
 * Unregisters a native function.
 *
 * @param name The name of the native function.
 */
void PythonLanguage::UnregisterNativeFunction(const String& name)
{
	ObjectLock olock(this);

	PyThreadState *tstate = PyThreadState_Swap(m_MainThreadState);

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

	(void) PyThreadState_Swap(tstate);
}

PyObject *PythonLanguage::PyRegisterFunction(PyObject *, PyObject *args)
{
	char *name;
	PyObject *object;

	if (!PyArg_ParseTuple(args, "sO", &name, &object))
		return NULL;

	PythonInterpreter *interp = GetCurrentInterpreter();

	if (interp == NULL) {
		PyErr_SetString(PyExc_RuntimeError, "GetCurrentInterpreter() returned NULL.");
		return NULL;
	}

	if (!PyCallable_Check(object)) {
		PyErr_SetString(PyExc_RuntimeError, "Function object is not callable.");
		return NULL;
	}

	interp->RegisterPythonFunction(name, object);

	Py_INCREF(Py_None);
	return Py_None;
}

/**
 * Retrieves the current interpreter object. Caller must hold the GIL.
 *
 * @returns The current interpreter.
 */
PythonInterpreter *PythonLanguage::GetCurrentInterpreter(void)
{
	return m_CurrentInterpreter;
}

/**
 * Sets the current interpreter. Caller must hold the GIL.
 *
 * @param interpreter The interpreter.
 */
void PythonLanguage::SetCurrentInterpreter(PythonInterpreter *interpreter)
{
	m_CurrentInterpreter = interpreter;
}
