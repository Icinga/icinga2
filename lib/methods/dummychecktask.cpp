/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#ifndef _WIN32
#	include <stdlib.h>
#endif /* _WIN32 */
#include "methods/dummychecktask.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/pluginutility.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/convert.hpp"
#include "base/function.hpp"
#include "base/logger.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, DummyCheck, &DummyCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void DummyCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	CheckCommand::Ptr command = CheckCommand::ExecuteOverride ? CheckCommand::ExecuteOverride : checkable->GetCheckCommand();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (MacroResolver::OverrideMacros)
		resolvers.emplace_back("override", MacroResolver::OverrideMacros);

	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", command);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	int dummyState = MacroProcessor::ResolveMacros("$dummy_state$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	String dummyText = MacroProcessor::ResolveMacros("$dummy_text$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	if (resolvedMacros && !useResolvedMacros)
		return;

	/* Parse output and performance data. */
	std::pair<String, String> co = PluginUtility::ParseCheckOutput(dummyText);

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
		cr->SetOutput(co.first);
		cr->SetPerformanceData(PluginUtility::SplitPerfdata(co.second));
		cr->SetState(PluginUtility::ExitStatusToState(dummyState));
		cr->SetExitStatus(dummyState);
		cr->SetExecutionStart(now);
		cr->SetExecutionEnd(now);
		cr->SetCommand(commandName);

		checkable->ProcessCheckResult(cr);
	}
}
