/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "base/datetime.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"
#include "base/objectlock.hpp"

using namespace icinga;

static String DateTimeFormat(const String& format)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	DateTime::Ptr self = static_cast<DateTime::Ptr>(vframe->Self);
	REQUIRE_NOT_NULL(self);

	return self->Format(format);
}

Object::Ptr DateTime::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "format", new Function("DateTime#format", DateTimeFormat, { "format" }) }
	});

	return prototype;
}
