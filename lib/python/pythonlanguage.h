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

#ifndef PYTHONLANGUAGE_H
#define PYTHONLANGUAGE_H

#include <Python.h>
#include "base/scriptlanguage.h"

namespace icinga
{

class PythonInterpreter;

/**
 * The Python scripting language.
 *
 * @ingroup base
 */
class PythonLanguage : public ScriptLanguage
{
public:
	DECLARE_PTR_TYPEDEFS(PythonLanguage);

	PythonLanguage(void);
	~PythonLanguage(void);

	virtual ScriptInterpreter::Ptr CreateInterpreter(const Script::Ptr& script);

	PyThreadState *GetMainThreadState(void) const;

	static PythonInterpreter *GetCurrentInterpreter(void);
	static void SetCurrentInterpreter(PythonInterpreter *interpreter);

	static PyObject *MarshalToPython(const Value& value);
	static Value MarshalFromPython(PyObject *value);

	String ExceptionInfoToString(PyObject *type, PyObject *exc, PyObject *tb) const;

private:
	bool m_Initialized;
	PyThreadState *m_MainThreadState;
	PyObject *m_NativeModule;
	PyObject *m_TracebackModule;
	static PythonInterpreter *m_CurrentInterpreter;

	void RegisterNativeFunction(const String& name);
	void UnregisterNativeFunction(const String& name);

	static PyObject *PyCallNativeFunction(PyObject *self, PyObject *args);
	static PyObject *PyRegisterFunction(PyObject *self, PyObject *args);

	static PyMethodDef m_NativeMethodDef[];

	void InitializeOnce(void);
};

}

#endif /* PYTHONLANGUAGE_H */
