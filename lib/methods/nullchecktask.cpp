// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

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

REGISTER_FUNCTION_NONCONST(Internal, NullCheck, &NullCheckTask::ScriptFunc, "checkable:cr:producer:resolvedMacros:useResolvedMacros");

void NullCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	if (resolvedMacros && !useResolvedMacros)
		return;

	String output = "Hello from ";
	output += IcingaApplication::GetInstance()->GetNodeName();
	ServiceState state = ServiceOK;

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		double now = Utility::GetTime();
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = output;
		pr.ExecutionStart = now;
		pr.ExecutionEnd = now;
		pr.ExitStatus = state;

		Checkable::ExecuteCommandProcessFinishedHandler("", pr);
	} else {
		cr->SetOutput(output);
		cr->SetPerformanceData(new Array({
			new PerfdataValue("time", Convert::ToDouble(Utility::GetTime()))
		}));
		cr->SetState(state);

		checkable->ProcessCheckResult(cr, producer);
	}
}
