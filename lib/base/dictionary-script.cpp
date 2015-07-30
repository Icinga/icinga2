/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2015 Icinga Development Team (http://www.icinga.org)    *
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
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
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

static Value DictionaryGet(const String& key)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	return self->Get(key);
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

static Dictionary::Ptr DictionaryClone(void)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	return self->ShallowClone();
}

Object::Ptr Dictionary::GetPrototype(void)
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("len", new Function(WrapFunction(DictionaryLen), true));
		prototype->Set("set", new Function(WrapFunction(DictionarySet)));
		prototype->Set("get", new Function(WrapFunction(DictionaryGet)));
		prototype->Set("remove", new Function(WrapFunction(DictionaryRemove)));
		prototype->Set("contains", new Function(WrapFunction(DictionaryContains), true));
		prototype->Set("clone", new Function(WrapFunction(DictionaryClone), true));
	}

	return prototype;
}

