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

	//PySys_SetArgv(argc, argv);

	m_MainThreadState = PyThreadState_Get();

	PyThreadState_Swap(NULL);

	PyEval_ReleaseLock();
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
