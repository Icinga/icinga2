// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
	REQUIRE_NOT_NULL(self);
	return self->GetLength();
}

static void DictionarySet(const String& key, const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	self->Set(key, value);
}

static Value DictionaryGet(const String& key)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	return self->Get(key);
}

static void DictionaryRemove(const String& key)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	self->Remove(key);
}

static void DictionaryClear()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	self->Clear();
}

static bool DictionaryContains(const String& key)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	return self->Contains(key);
}

static Dictionary::Ptr DictionaryShallowClone()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	return self->ShallowClone();
}

static Array::Ptr DictionaryKeys()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	ArrayData keys;
	ObjectLock olock(self);
	for (const Dictionary::Pair& kv : self) {
		keys.push_back(kv.first);
	}
	return new Array(std::move(keys));
}

static Array::Ptr DictionaryValues()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	ArrayData values;
	ObjectLock olock(self);
	for (const Dictionary::Pair& kv : self) {
		values.push_back(kv.second);
	}
	return new Array(std::move(values));
}

static void DictionaryFreeze()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Dictionary::Ptr self = static_cast<Dictionary::Ptr>(vframe->Self);
	self->Freeze();
}

Object::Ptr Dictionary::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "len", new Function("Dictionary#len", DictionaryLen, {}, true) },
		{ "set", new Function("Dictionary#set", DictionarySet, { "key", "value" }) },
		{ "get", new Function("Dictionary#get", DictionaryGet, { "key" }) },
		{ "remove", new Function("Dictionary#remove", DictionaryRemove, { "key" }) },
		{ "clear", new Function("Dictionary#clear", DictionaryClear, {}) },
		{ "contains", new Function("Dictionary#contains", DictionaryContains, { "key" }, true) },
		{ "shallow_clone", new Function("Dictionary#shallow_clone", DictionaryShallowClone, {}, true) },
		{ "keys", new Function("Dictionary#keys", DictionaryKeys, {}, true) },
		{ "values", new Function("Dictionary#values", DictionaryValues, {}, true) },
		{ "freeze", new Function("Dictionary#freeze", DictionaryFreeze, {}) }
	});

	return prototype;
}
