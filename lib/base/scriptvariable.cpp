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

Registry<ScriptVariable, Value> ScriptVariable::m_Registry;

Value ScriptVariable::Get(const String& name)
{
	Value value = m_Registry.GetItem(name);
	if (value.IsEmpty())
		Log(LogWarning, "icinga", "Tried to access empty variable: " + name);

	return value;
}

void ScriptVariable::Set(const String& name, const Value& value)
{
	m_Registry.Register(name, value);
}
