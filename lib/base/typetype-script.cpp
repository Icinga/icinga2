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

	int fid = self->GetFieldId(fieldName);
	self->RegisterAttributeHandler(fid, std::bind(&InvokeAttributeHandlerHelper, callback, _1, _2));
}

Object::Ptr TypeType::GetPrototype()
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("register_attribute_handler", new Function("Type#register_attribute_handler", TypeRegisterAttributeHandler, { "field", "callback" }, false));
	}

	return prototype;
}

