// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/objectlock.hpp"
#include "base/exception.hpp"

using namespace icinga;

static Value FunctionCall(const std::vector<Value>& args)
{
	if (args.size() < 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for call()"));

	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Function::Ptr self = static_cast<Function::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	std::vector<Value> uargs(args.begin() + 1, args.end());
	return self->InvokeThis(args[0], uargs);
}

static Value FunctionCallV(const Value& thisArg, const Array::Ptr& args)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Function::Ptr self = static_cast<Function::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	std::vector<Value> uargs;

	{
		ObjectLock olock(args);
		uargs = std::vector<Value>(args->Begin(), args->End());
	}

	return self->InvokeThis(thisArg, uargs);
}


Object::Ptr Function::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "call", new Function("Function#call", FunctionCall) },
		{ "callv", new Function("Function#callv", FunctionCallV) }
	});

	return prototype;
}
