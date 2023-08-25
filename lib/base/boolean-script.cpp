/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

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
