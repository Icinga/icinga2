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

#ifndef PYTHONINTERPRETER_H
#define PYTHONINTERPRETER_H

namespace icinga
{

/**
 * A Python script interpreter.
 *
 * @ingroup base
 */
class I2_PYTHON_API PythonInterpreter : public ScriptInterpreter
{
public:
	typedef shared_ptr<PythonInterpreter> Ptr;
	typedef weak_ptr<PythonInterpreter> WeakPtr;

	PythonInterpreter(const PythonLanguage::Ptr& language, const Script::Ptr& script);
	~PythonInterpreter(void);

	void RegisterFunction(const String& name, PyObject *function);
	void UnregisterFunction(const String& name);

protected:
	PythonLanguage::Ptr m_Language;
	PyThreadState *m_ThreadState;
	map<String, PyObject *> m_Functions;

	virtual void ProcessCall(const String& function, const ScriptTask::Ptr& task,
	    const vector<Value>& arguments);
};

}

#endif /* PYTHONINTERPRETER_H */
