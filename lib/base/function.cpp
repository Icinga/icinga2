/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
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

#include "base/function.hpp"
#include "base/function.tcpp"
#include "base/array.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

REGISTER_TYPE_WITH_PROTOTYPE(Function, Function::GetPrototype());

Function::Function(const String& name, const Callback& function, const std::vector<String>& args,
	bool side_effect_free, bool deprecated)
	: m_Callback(function)
{
	SetName(name, true);
	SetSideEffectFree(side_effect_free, true);
	SetDeprecated(deprecated, true);
	SetArguments(Array::FromVector(args), true);
}

Value Function::Invoke(const std::vector<Value>& arguments)
{
	ScriptFrame frame(false);
	return m_Callback(arguments);
}

Value Function::InvokeThis(const Value& otherThis, const std::vector<Value>& arguments)
{
	ScriptFrame frame(false, otherThis);
	return m_Callback(arguments);
}

Object::Ptr Function::Clone() const
{
	return const_cast<Function *>(this);
}
