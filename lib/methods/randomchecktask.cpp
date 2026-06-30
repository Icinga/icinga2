// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/randomchecktask.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/checkcommand.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, RandomCheck, &RandomCheckTask::ScriptFunc, "checkable:cr:producer:resolvedMacros:useResolvedMacros");

void RandomCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	if (resolvedMacros && !useResolvedMacros)
		return;

	double now = Utility::GetTime();
	double uptime = Application::GetUptime();

	String output = "Hello from " + IcingaApplication::GetInstance()->GetNodeName()
		+ ". Icinga 2 has been running for " + Utility::FormatDuration(uptime)
		+ ". Version: " + Application::GetAppVersion();

	CheckCommand::Ptr command = CheckCommand::ExecuteOverride ? CheckCommand::ExecuteOverride : checkable->GetCheckCommand();
	String commandName = command->GetName();
	ServiceState state = static_cast<ServiceState>(Utility::Random() % 4);

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		double now = Utility::GetTime();
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = output;
		pr.ExecutionStart = now;
		pr.ExecutionEnd = now;
		pr.ExitStatus = state;

		Checkable::ExecuteCommandProcessFinishedHandler(commandName, pr);
	} else {
		cr->SetOutput(output);

		double random = Utility::Random() % 1000;
		cr->SetPerformanceData(new Array({
			new PerfdataValue("time", now),
			new PerfdataValue("value", random),
			new PerfdataValue("value_1m", random * 0.9),
			new PerfdataValue("value_5m", random * 0.8),
			new PerfdataValue("uptime", uptime),
		}));

		cr->SetState(state);
		cr->SetCommand(commandName);

		checkable->ProcessCheckResult(cr, producer);
	}
}
