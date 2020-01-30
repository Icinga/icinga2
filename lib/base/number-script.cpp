/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/number.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

static String NumberToString()
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	return vframe->Self;
}

Object::Ptr Number::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "to_string", new Function("Number#to_string", NumberToString, {}, true) }
	});

	return prototype;
}
