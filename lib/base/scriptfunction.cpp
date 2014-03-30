/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/scriptfunction.h"
#include "base/scriptvariable.h"
#include "base/registry.h"
#include "base/singleton.h"

using namespace icinga;

ScriptFunction::ScriptFunction(const Callback& function)
	: m_Callback(function)
{ }

Value ScriptFunction::Invoke(const std::vector<Value>& arguments)
{
	return m_Callback(arguments);
}

ScriptFunction::Ptr ScriptFunction::GetByName(const String& name)
{
	return ScriptFunctionRegistry::GetInstance()->GetItem(name);
}

void ScriptFunction::Register(const String& name, const ScriptFunction::Callback& function)
{
	ScriptVariable::Ptr sv = ScriptVariable::Set(name, name);
	sv->SetConstant(true);

	ScriptFunction::Ptr func = make_shared<ScriptFunction>(function);
	ScriptFunctionRegistry::GetInstance()->Register(name, func);
}

void ScriptFunction::Unregister(const String& name)
{
	ScriptVariable::Unregister(name);
	ScriptFunctionRegistry::GetInstance()->Unregister(name);
}

RegisterFunctionHelper::RegisterFunctionHelper(const String& name, const ScriptFunction::Callback& function)
{
	ScriptFunction::Register(name, function);
}

ScriptFunctionRegistry *ScriptFunctionRegistry::GetInstance(void)
{
	return Singleton<ScriptFunctionRegistry>::GetInstance();
}

