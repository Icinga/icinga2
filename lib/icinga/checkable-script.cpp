/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "icinga/checkable.hpp"
#include "base/configobject.hpp"
#include "base/dictionary.hpp"
#include "base/function.hpp"
#include "base/functionwrapper.hpp"
#include "base/scriptframe.hpp"

using namespace icinga;

static void CheckableProcessCheckResult(const CheckResult::Ptr& cr)
{
	ScriptFrame *vframe = ScriptFrame::GetCurrentFrame();
	Checkable::Ptr self = vframe->Self;
	REQUIRE_NOT_NULL(self);
	self->ProcessCheckResult(cr);
}

Object::Ptr Checkable::GetPrototype()
{
	static Dictionary::Ptr prototype = new Dictionary({
		{ "process_check_result", new Function("Checkable#process_check_result", CheckableProcessCheckResult, { "cr" }, false) }
	});

	return prototype;
}

