/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/object.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

static String ObjectToString()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Object::Ptr self = static_cast<Object::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	return self->ToString();
}

static void ObjectNotifyAttribute(const String& attribute)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Object::Ptr self = static_cast<Object::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	self->NotifyField(self->GetReflectionType()->GetFieldId(attribute));
}

static Object::Ptr ObjectClone()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Object::Ptr self = static_cast<Object::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	return self->Clone();
}

Object::Ptr Object::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
			{ "to_string", new Function("Object#to_string", ObjectToString, {}, true) },
			{ "notify_attribute", new Function("Object#notify_attribute", ObjectNotifyAttribute, { "attribute" }, false) },
			{ "clone", new Function("Object#clone", ObjectClone, {}, true) }
	});

	return prototype;
}
