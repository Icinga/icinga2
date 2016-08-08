/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2016 Icinga Development Team (https://www.icinga.org/)  *
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
#include "base/primitivetype.hpp"
#include "base/dictionary.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

REGISTER_PRIMITIVE_TYPE_NOINST(Function, Object, Function::GetPrototype());

Function::Function(const Callback& function, bool side_effect_free)
	: m_Callback(function), m_SideEffectFree(side_effect_free)
{ }

Value Function::Invoke(const std::vector<Value>& arguments)
{
	ScriptFrame frame;
	return m_Callback(arguments);
}

Value Function::Invoke(const Value& otherThis, const std::vector<Value>& arguments)
{
	ScriptFrame frame;
	frame.Self = otherThis;
	return m_Callback(arguments);
}

bool Function::IsSideEffectFree(void) const
{
	return m_SideEffectFree;
}

Object::Ptr Function::Clone(void) const
{
	return const_cast<Function *>(this);
}
