// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "methods/sleepchecktask.hpp"
#include "icinga/pluginutility.hpp"
#include "base/utility.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, SleepCheck, &SleepCheckTask::ScriptFunc, "checkable:cr:producer:resolvedMacros:useResolvedMacros");

void SleepCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
    REQUIRE_NOT_NULL(checkable);
    REQUIRE_NOT_NULL(cr);

	CheckCommand::Ptr commandObj = CheckCommand::ExecuteOverride ? CheckCommand::ExecuteOverride : checkable->GetCheckCommand();

    Host::Ptr host;
    Service::Ptr service;
    tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;

	if (MacroResolver::OverrideMacros)
		resolvers.emplace_back("override", MacroResolver::OverrideMacros);

    if (service)
        resolvers.emplace_back("service", service);
    resolvers.emplace_back("host", host);
    resolvers.emplace_back("command", commandObj);

    double sleepTime = MacroProcessor::ResolveMacros("$sleep_time$", resolvers, checkable->GetLastCheckResult(),
            nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

    if (resolvedMacros && !useResolvedMacros)
        return;

    Utility::Sleep(sleepTime);

    String output = "Slept for " + Convert::ToString(sleepTime) + " seconds.";

    double now = Utility::GetTime();
	CheckCommand::Ptr command = checkable->GetCheckCommand();

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = output;
		pr.ExecutionStart = now - sleepTime;
		pr.ExecutionEnd = now;
		pr.ExitStatus = 0;

		Checkable::ExecuteCommandProcessFinishedHandler("", pr);
	} else {
		cr->SetOutput(output);
		cr->SetExecutionStart(now);
		cr->SetExecutionEnd(now);
		cr->SetCommand(command->GetName());

		checkable->ProcessCheckResult(cr, producer);
	}
}
