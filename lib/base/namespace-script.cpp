/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
	for (const auto& kv : self) {
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
	for (const auto& kv : self) {
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

