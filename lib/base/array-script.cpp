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

#include "base/array.hpp"
#include "base/scriptfunction.hpp"
#include "base/scriptfunctionwrapper.hpp"
#include "config/vmframe.hpp"

using namespace icinga;

static double ArrayLen(void)
{
	VMFrame *vframe = VMFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	return self->GetLength();
}

static void ArraySet(int index, const Value& value)
{
	VMFrame *vframe = VMFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->Set(index, value);
}

static void ArrayAdd(const Value& value)
{
	VMFrame *vframe = VMFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->Add(value);
}

static void ArrayRemove(int index)
{
	VMFrame *vframe = VMFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->Remove(index);
}

static bool ArrayContains(const Value& value)
{
	VMFrame *vframe = VMFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	return self->Contains(value);
}

static void ArrayClear(void)
{
	VMFrame *vframe = VMFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->Clear();
}

static void ArrayClone(void)
{
	VMFrame *vframe = VMFrame::GetCurrentFrame();
	Array::Ptr self = static_cast<Array::Ptr>(vframe->Self);
	self->ShallowClone();
}

Object::Ptr Array::GetPrototype(void)
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("len", new ScriptFunction(WrapScriptFunction(ArrayLen)));
		prototype->Set("set", new ScriptFunction(WrapScriptFunction(ArraySet)));
		prototype->Set("add", new ScriptFunction(WrapScriptFunction(ArrayAdd)));
		prototype->Set("remove", new ScriptFunction(WrapScriptFunction(ArrayRemove)));
		prototype->Set("contains", new ScriptFunction(WrapScriptFunction(ArrayContains)));
		prototype->Set("clear", new ScriptFunction(WrapScriptFunction(ArrayClear)));
		prototype->Set("clone", new ScriptFunction(WrapScriptFunction(ArrayClone)));
	}

	return prototype;
}

