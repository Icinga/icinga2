// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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
