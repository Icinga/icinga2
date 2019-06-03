/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/nullchecktask.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, NullCheck, &NullCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void NullCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	if (resolvedMacros && !useResolvedMacros)
		return;

	String output = "Hello from ";
	output += IcingaApplication::GetInstance()->GetNodeName();

	cr->SetOutput(output);
	cr->SetPerformanceData(new Array({
		new PerfdataValue("time", Convert::ToDouble(Utility::GetTime()))
	}));
	cr->SetState(ServiceOK);

	checkable->ProcessCheckResult(cr);
}
