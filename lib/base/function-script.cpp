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

	std::vector<Value> uargs(args.begin() + 1, args.end());
	return self->InvokeThis(args[0], uargs);
}

static Value FunctionCallV(const Value& thisArg, const Array::Ptr& args)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Function::Ptr self = static_cast<Function::Ptr>(vframe->Self);

	std::vector<Value> uargs;

	{
		ObjectLock olock(args);
		uargs = std::vector<Value>(args->Begin(), args->End());
	}

	return self->InvokeThis(thisArg, uargs);
}


Object::Ptr Function::GetPrototype()
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("call", new Function("Function#call", FunctionCall));
		prototype->Set("callv", new Function("Function#callv", FunctionCallV));
	}

	return prototype;
}

