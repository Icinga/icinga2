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

signals2::signal<void (const String&, const ScriptFunction::Ptr&)> ScriptFunction::OnRegistered;
signals2::signal<void (const String&)> ScriptFunction::OnUnregistered;

ScriptFunction::ScriptFunction(const Callback& function)
	: m_Callback(function)
{ }

/**
 * @threadsafety Always.
 */
void ScriptFunction::Register(const String& name, const ScriptFunction::Ptr& function)
{
	boost::mutex::scoped_lock lock(GetMutex());

	InternalGetFunctions()[name] = function;
	OnRegistered(name, function);
}

/**
 * @threadsafety Always.
 */
void ScriptFunction::Unregister(const String& name)
{
	boost::mutex::scoped_lock lock(GetMutex());

	InternalGetFunctions().erase(name);
	OnUnregistered(name);
}

/**
 * @threadsafety Always.
 */
ScriptFunction::Ptr ScriptFunction::GetByName(const String& name)
{
	boost::mutex::scoped_lock lock(GetMutex());

	map<String, ScriptFunction::Ptr>::iterator it;

	it = InternalGetFunctions().find(name);

	if (it == InternalGetFunctions().end())
		return ScriptFunction::Ptr();

	return it->second;
}

/**
 * @threadsafety Always.
 */
void ScriptFunction::Invoke(const ScriptTask::Ptr& task, const vector<Value>& arguments)
{
	m_Callback(task, arguments);
}

/**
 * @threadsafety Always.
 */
map<String, ScriptFunction::Ptr> ScriptFunction::GetFunctions(void)
{
	boost::mutex::scoped_lock lock(GetMutex());

	return InternalGetFunctions(); /* makes a copy of the map */
}

/**
 * @threadsafety Caller must hold the mutex returned by GetMutex().
 */
map<String, ScriptFunction::Ptr>& ScriptFunction::InternalGetFunctions(void)
{
	static map<String, ScriptFunction::Ptr> functions;
	return functions;
}

/**
 * @threadsafety Always.
 */
boost::mutex& ScriptFunction::GetMutex(void)
{
	static boost::mutex mtx;
	return mtx;
}
