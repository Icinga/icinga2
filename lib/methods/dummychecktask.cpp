// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/dummychecktask.hpp"
#include "icinga/pluginutility.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, DummyCheck, &DummyCheckTask::ScriptFunc, "checkable:cr:producer:resolvedMacros:useResolvedMacros");

void DummyCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	CheckCommand::Ptr command = CheckCommand::ExecuteOverride ? CheckCommand::ExecuteOverride : checkable->GetCheckCommand();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	resolvers.reserve(5);

	if (MacroResolver::OverrideMacros)
		resolvers.emplace_back("override", MacroResolver::OverrideMacros);

	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", command);

	int dummyState = MacroProcessor::ResolveMacros("$dummy_state$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	String dummyText = MacroProcessor::ResolveMacros("$dummy_text$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	if (resolvedMacros && !useResolvedMacros)
		return;

	double now = Utility::GetTime();
	String commandName = command->GetName();

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = dummyText;
		pr.ExecutionStart = now;
		pr.ExecutionEnd = now;
		pr.ExitStatus = dummyState;

		Checkable::ExecuteCommandProcessFinishedHandler(commandName, pr);
	} else {
		/* Parse output and performance data. */
		std::pair<String, String> co = PluginUtility::ParseCheckOutput(dummyText);

		cr->SetOutput(co.first);
		cr->SetPerformanceData(PluginUtility::SplitPerfdata(co.second));
		cr->SetState(PluginUtility::ExitStatusToState(dummyState));
		cr->SetExitStatus(dummyState);
		cr->SetExecutionStart(now);
		cr->SetExecutionEnd(now);
		cr->SetCommand(commandName);

		checkable->ProcessCheckResult(cr, producer);
	}
}
