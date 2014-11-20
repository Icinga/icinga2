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

#include "base/scriptsignal.hpp"
#include "base/scriptvariable.hpp"

using namespace icinga;

void ScriptSignal::AddSlot(const Callback& slot)
{
	m_Slots.push_back(slot);
}

Value ScriptSignal::Invoke(const std::vector<Value>& arguments)
{
	BOOST_FOREACH(const Callback& slot, m_Slots)
		slot(arguments);
}

ScriptSignal::Ptr ScriptSignal::GetByName(const String& name)
{
	ScriptVariable::Ptr sv = ScriptVariable::GetByName(name);

	if (!sv)
		return ScriptSignal::Ptr();

	return sv->GetData();
}

void ScriptSignal::Register(const String& name, const ScriptSignal::Ptr& function)
{
	ScriptVariable::Ptr sv = ScriptVariable::Set(name, function);
	sv->SetConstant(true);
}

void ScriptSignal::Unregister(const String& name)
{
	ScriptVariable::Unregister(name);
}

