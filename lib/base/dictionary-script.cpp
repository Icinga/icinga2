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

#include "base/dictionary.hpp"
#include "base/scriptfunction.hpp"
#include "base/scriptfunctionwrapper.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

static double DictionaryLen(void)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	return self->GetLength();
}

static void DictionarySet(const String& key, const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	self->Set(key, value);
}

static void DictionaryRemove(const String& key)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	self->Remove(key);
}

static bool DictionaryContains(const String& key)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	return self->Contains(key);
}

static void DictionaryClone(void)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	self->ShallowClone();
}

Object::Ptr Dictionary::GetPrototype(void)
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("len", new ScriptFunction(WrapScriptFunction(DictionaryLen)));
		prototype->Set("set", new ScriptFunction(WrapScriptFunction(DictionarySet)));
		prototype->Set("remove", new ScriptFunction(WrapScriptFunction(DictionaryRemove)));
		prototype->Set("contains", new ScriptFunction(WrapScriptFunction(DictionaryContains)));
		prototype->Set("clone", new ScriptFunction(WrapScriptFunction(DictionaryClone)));
	}

	return prototype;
}

