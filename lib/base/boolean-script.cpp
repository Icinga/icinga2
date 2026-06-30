// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "base/boolean.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

static String BooleanToString()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	bool self = vframe->Self;
	return self ? "true" : "false";
}

Object::Ptr Boolean::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "to_string", new Function("Boolean#to_string", BooleanToString, {}, true) }
	});

	return prototype;
}
