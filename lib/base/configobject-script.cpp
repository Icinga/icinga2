/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/configobject.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/reference.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

static bool ConfigObjectGetAttribute(const String& attr, const Reference::Ptr& result)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	ConfigObject::Ptr self = vframe->Self;

	REQUIRE_NOT_NULL(self);

	Value temp;
	bool ok = self->GetAttribute(attr, &temp);

	if (ok && result) {
		result->Set(temp);
	}

	return ok;
}

static void ConfigObjectModifyAttribute(const String& attr, const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	ConfigObject::Ptr self = vframe->Self;
	REQUIRE_NOT_NULL(self);
	return self->ModifyAttribute(attr, value);
}

static void ConfigObjectRestoreAttribute(const String& attr)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	ConfigObject::Ptr self = vframe->Self;
	REQUIRE_NOT_NULL(self);
	return self->RestoreAttribute(attr);
}

Object::Ptr ConfigObject::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "get_attribute", new Function("ConfigObject#get_attribute", ConfigObjectGetAttribute, { "attr", "result" }, false) },
		{ "modify_attribute", new Function("ConfigObject#modify_attribute", ConfigObjectModifyAttribute, { "attr", "value" }, false) },
		{ "restore_attribute", new Function("ConfigObject#restore_attribute", ConfigObjectRestoreAttribute, { "attr", "value" }, false) }
	});

	return prototype;
}

