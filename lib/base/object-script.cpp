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
	return self->ToString();
}

static void ObjectNotifyAttribute(const String& attribute)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Object::Ptr self = static_cast<Object::Ptr>(vframe->Self);
	self->NotifyField(self->GetReflectionType()->GetFieldId(attribute));
}

static Object::Ptr ObjectClone()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Object::Ptr self = static_cast<Object::Ptr>(vframe->Self);
	return self->Clone();
}

Object::Ptr Object::GetPrototype()
{
	static Dictionary::Ptr prototype;

	if (!prototype) {
		prototype = new Dictionary();
		prototype->Set("to_string", new Function("Object#to_string", ObjectToString, {}, true));
		prototype->Set("notify_attribute", new Function("Object#notify_attribute", ObjectNotifyAttribute, { "attribute" }, false));
		prototype->Set("clone", new Function("Object#clone", ObjectClone, {}, true));
	}

	return prototype;
}

