/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2014 Icinga Development Team (http://www.icinga.org)    *
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

#include "base/scriptfunction.hpp"
#include "base/scriptfunctionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/exception.hpp"

using namespace icinga;

static Value ScriptFunctionCall(const std::vector<Value>& args)
{
	if (args.size() < 1)
		BOOST_THROW_EXCEPTION(std::invalid_argument("Too few arguments for call()"));

	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	ScriptFunction::Ptr self = static_cast<ScriptFunction::Ptr>(vframe->Self);

	ScriptFrame uframe(args[0]);
	std::vector<Value> uargs(args.begin() + 1, args.end());
	return self->Invoke(uargs);
}

Object::Ptr ScriptFunction::GetPrototype(void)
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("call", new ScriptFunction(WrapScriptFunction(ScriptFunctionCall)));
	}

	return prototype;
}

