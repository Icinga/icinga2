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

#include "base/scriptvariable.h"
#include "base/logger_fwd.h"

using namespace icinga;

ScriptVariable::ScriptVariable(const Value& data)
	: m_Data(data), m_Constant(false)
{ }

ScriptVariable::Ptr ScriptVariable::GetByName(const String& name)
{
	return ScriptVariableRegistry::GetInstance()->GetItem(name);
}

void ScriptVariable::SetConstant(bool constant)
{
	m_Constant = constant;
}

bool ScriptVariable::IsConstant(void) const
{
	return m_Constant;
}

void ScriptVariable::SetData(const Value& data)
{
	m_Data = data;
}

Value ScriptVariable::GetData(void) const
{
	return m_Data;
}

Value ScriptVariable::Get(const String& name)
{
	ScriptVariable::Ptr sv = GetByName(name);

	if (!sv) {
		Log(LogWarning, "icinga", "Tried to access undefined variable: " + name);
		return Empty;
	}

	return sv->GetData();
}

ScriptVariable::Ptr ScriptVariable::Set(const String& name, const Value& value, bool overwrite, bool make_const)
{
	ScriptVariable::Ptr sv = GetByName(name);

	if (!sv) {
		sv = make_shared<ScriptVariable>(value);
		ScriptVariableRegistry::GetInstance()->Register(name, sv);
	} else if (overwrite) {
		if (sv->IsConstant())
			BOOST_THROW_EXCEPTION(std::invalid_argument("Tried to modify read-only script variable '" + name + "'"));

		sv->SetData(value);
	}

	if (make_const)
		sv->SetConstant(true);

	return sv;
}

ScriptVariableRegistry *ScriptVariableRegistry::GetInstance(void)
{
	return Singleton<ScriptVariableRegistry>::GetInstance();
}

