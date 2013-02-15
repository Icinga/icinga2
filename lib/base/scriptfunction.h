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

#ifndef SCRIPTFUNCTION_H
#define SCRIPTFUNCTION_H

namespace icinga
{

class ScriptTask;

/**
 * A type hint.
 */
struct ScriptArgumentHint
{
	bool RestrictType;
	ValueType Type;
	String Class;

	ScriptArgumentHint(void)
		: RestrictType(false), Type(ValueEmpty), Class()
	{ }
};

/**
 * A script function that can be used to execute a script task.
 *
 * @ingroup base
 */
class I2_BASE_API ScriptFunction : public Object
{
public:
	typedef shared_ptr<ScriptFunction> Ptr;
	typedef weak_ptr<ScriptFunction> WeakPtr;

	typedef function<void (const shared_ptr<ScriptTask>&, const vector<Value>& arguments)> Callback;

	ScriptFunction(const Callback& function);

	static void Register(const String& name, const ScriptFunction::Ptr& function);
	static void Unregister(const String& name);
	static ScriptFunction::Ptr GetByName(const String& name);

	void Invoke(const shared_ptr<ScriptTask>& task, const vector<Value>& arguments);

	void SetArgumentCount(int count);
	int GetArgumentCount(void) const;

	void SetArgumentHint(int index, const ScriptArgumentHint& hint);
	ScriptArgumentHint GetArgumentHint(int index) const;

	void SetReturnHint(const ScriptArgumentHint& hint);
	ScriptArgumentHint GetReturnHint(void) const;

	static map<String, ScriptFunction::Ptr>& GetFunctions(void);

	static boost::signal<void (const String&, const ScriptFunction::Ptr&)> OnRegistered;
	static boost::signal<void (const String&)> OnUnregistered;

private:
	Callback m_Callback;
	int m_ArgumentCount;
	vector<ScriptArgumentHint> m_ArgumentHints;
	ScriptArgumentHint m_ReturnHint;
};

/**
 * Helper class for registering ScriptFunction implementation classes.
 *
 * @ingroup base
 */
class RegisterFunctionHelper
{
public:
	RegisterFunctionHelper(const String& name, const ScriptFunction::Callback& function)
	{
		if (!ScriptFunction::GetByName(name)) {
			ScriptFunction::Ptr func = boost::make_shared<ScriptFunction>(function);
			ScriptFunction::Register(name, func);
		}
	}
};

#define REGISTER_SCRIPTFUNCTION(name, callback) \
	static RegisterFunctionHelper g_RegisterSF_ ## type(name, callback)

}

#endif /* SCRIPTFUNCTION_H */
