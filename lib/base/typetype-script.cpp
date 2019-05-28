/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/type.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

static void InvokeAttributeHandlerHelper(const Function::Ptr& callback,
	const Object::Ptr& object, const Value& cookie)
{
	callback->Invoke({ object });
}

static void TypeRegisterAttributeHandler(const String& fieldName, const Function::Ptr& callback)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Type::Ptr self = static_cast<Type::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	int fid = self->GetFieldId(fieldName);
	auto lambdaInvokeAttributeHandlerHelper = [&](const Object::Ptr& object, const Value& cookie){return InvokeAttributeHandlerHelper(callback, object, cookie);};
	self->RegisterAttributeHandler(fid, lambdaInvokeAttributeHandlerHelper);
}

Object::Ptr TypeType::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "register_attribute_handler", new Function("Type#register_attribute_handler", TypeRegisterAttributeHandler, { "field", "callback" }, false) }
	});

	return prototype;
}

