/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "base/namespace.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/array.hpp"

using namespace icinga;

static void NamespaceSet(const String& key, const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Namespace::Ptr self = static_cast<Namespace::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	self->Set(key, value);
}

static Value NamespaceGet(const String& key)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Namespace::Ptr self = static_cast<Namespace::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	return self->Get(key);
}

static void NamespaceRemove(const String& key)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Namespace::Ptr self = static_cast<Namespace::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	self->Remove(key);
}

static bool NamespaceContains(const String& key)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Namespace::Ptr self = static_cast<Namespace::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	return self->Contains(key);
}

static Array::Ptr NamespaceKeys()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Namespace::Ptr self = static_cast<Namespace::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	ArrayData keys;
	ObjectLock olock(self);
	for (const Namespace::Pair& kv : self) {
		keys.push_back(kv.first);
	}
	return new Array(std::move(keys));
}

static Array::Ptr NamespaceValues()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Namespace::Ptr self = static_cast<Namespace::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	ArrayData values;
	ObjectLock olock(self);
	for (const Namespace::Pair& kv : self) {
		values.push_back(kv.second->Get());
	}
	return new Array(std::move(values));
}

Object::Ptr Namespace::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "set", new Function("Namespace#set", NamespaceSet, { "key", "value" }) },
		{ "get", new Function("Namespace#get", NamespaceGet, { "key" }) },
		{ "remove", new Function("Namespace#remove", NamespaceRemove, { "key" }) },
		{ "contains", new Function("Namespace#contains", NamespaceContains, { "key" }, true) },
		{ "keys", new Function("Namespace#keys", NamespaceKeys, {}, true) },
		{ "values", new Function("Namespace#values", NamespaceValues, {}, true) },
	});

	return prototype;
}

