/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://icinga.com/)      *
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

#include "base/reference.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/exception.hpp"

using namespace icinga;

static void ReferenceSet(const Value& value)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Reference::Ptr self = static_cast<Reference::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	self->Set(value);
}

static Value ReferenceGet()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Reference::Ptr self = static_cast<Reference::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);
	return self->Get();
}

Object::Ptr Reference::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "set", new Function("Reference#set", ReferenceSet, { "value" }) },
		{ "get", new Function("Reference#get", ReferenceGet, {}, true) },
	});

	return prototype;
}
