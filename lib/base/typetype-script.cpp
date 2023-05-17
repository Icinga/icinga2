/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/type.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

static void TypeRegisterAttributeHandler(const String& fieldName, const Function::Ptr& callback)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Type::Ptr self = static_cast<Type::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	int fid = self->GetFieldId(fieldName);
	self->RegisterAttributeHandler(fid, [callback](const Object::Ptr& object, const Value& cookie) {
		callback->Invoke({ object });
	});
}

Object::Ptr TypeType::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "register_attribute_handler", new Function("Type#register_attribute_handler", TypeRegisterAttributeHandler, { "field", "callback" }, false) }
	});

	return prototype;
}
