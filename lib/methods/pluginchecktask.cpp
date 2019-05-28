/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "methods/pluginchecktask.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/process.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, PluginCheck,  &PluginCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void PluginCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", commandObj);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	int timeout = commandObj->GetTimeout();

	if (!checkable->GetCheckTimeout().IsEmpty())
		timeout = checkable->GetCheckTimeout();

	auto lambdaProcessFinishedHandler = [&, checkable, cr](const Value& commandLine, const ProcessResult& pr){
		return PluginCheckTask::ProcessFinishedHandler(checkable, cr, commandLine, pr);
	};
	PluginUtility::ExecuteCommand(commandObj, checkable, checkable->GetLastCheckResult(),
		resolvers, resolvedMacros, useResolvedMacros, timeout, lambdaProcessFinishedHandler);

	if (!resolvedMacros || useResolvedMacros)
		Checkable::IncreasePendingChecks();
}

void PluginCheckTask::ProcessFinishedHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const Value& commandLine, const ProcessResult& pr)
{
	Checkable::DecreasePendingChecks();

	if (pr.ExitStatus > 3) {
		Process::Arguments parguments = Process::PrepareCommand(commandLine);
		Log(LogWarning, "PluginCheckTask")
			<< "Check command for object '" << checkable->GetName() << "' (PID: " << pr.PID
			<< ", arguments: " << Process::PrettyPrintArguments(parguments) << ") terminated with exit code "
			<< pr.ExitStatus << ", output: " << pr.Output;
	}

	String output = pr.Output.Trim();

	std::pair<String, String> co = PluginUtility::ParseCheckOutput(output);
	cr->SetCommand(commandLine);
	cr->SetOutput(co.first);
	cr->SetPerformanceData(PluginUtility::SplitPerfdata(co.second));
	cr->SetState(PluginUtility::ExitStatusToState(pr.ExitStatus));
	cr->SetExitStatus(pr.ExitStatus);
	cr->SetExecutionStart(pr.ExecutionStart);
	cr->SetExecutionEnd(pr.ExecutionEnd);

	checkable->ProcessCheckResult(cr);
}
