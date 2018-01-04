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

#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/array.hpp"

using namespace icinga;

static double DictionaryLen()
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

static Dictionary::Ptr DictionaryShallowClone()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	return self->ShallowClone();
}

static Array::Ptr DictionaryKeys()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	Array::Ptr keys = new Array();
	ObjectLock olock(self);
	for (const Dictionary::Pair& kv : self) {
		keys->Add(kv.first);
	}
	return keys;
}

static Array::Ptr DictionaryValues()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	Array::Ptr keys = new Array();
	ObjectLock olock(self);
	for (const Dictionary::Pair& kv : self) {
		keys->Add(kv.second);
	}
	return keys;
}

Object::Ptr Dictionary::GetPrototype()
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("len", new Function("Dictionary#len", DictionaryLen, {}, true));
		prototype->Set("set", new Function("Dictionary#set", DictionarySet, { "key", "value" }));
		prototype->Set("get", new Function("Dictionary#get", DictionaryGet, { "key" }));
		prototype->Set("remove", new Function("Dictionary#remove", DictionaryRemove, { "key" }));
		prototype->Set("contains", new Function("Dictionary#contains", DictionaryContains, { "key" }, true));
		prototype->Set("shallow_clone", new Function("Dictionary#shallow_clone", DictionaryShallowClone, {}, true));
		prototype->Set("keys", new Function("Dictionary#keys", DictionaryKeys, {}, true));
		prototype->Set("values", new Function("Dictionary#values", DictionaryValues, {}, true));
	}

	return prototype;
}

