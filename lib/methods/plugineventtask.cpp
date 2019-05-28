/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "methods/plugineventtask.hpp"
#include "icinga/eventcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/process.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, PluginEvent, &PluginEventTask::ScriptFunc, "checkable:resolvedMacros:useResolvedMacros");

void PluginEventTask::ScriptFunc(const Checkable::Ptr& checkable,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);

	EventCommand::Ptr commandObj = checkable->GetEventCommand();

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

	auto lambdaProcessFinishedHandler = [&, checkable](const Value& commandLine, const ProcessResult& pr){
		return PluginEventTask::ProcessFinishedHandler(checkable, commandLine, pr);
	};
	PluginUtility::ExecuteCommand(commandObj, checkable, checkable->GetLastCheckResult(),
		resolvers, resolvedMacros, useResolvedMacros, timeout, lambdaProcessFinishedHandler);
}

void PluginEventTask::ProcessFinishedHandler(const Checkable::Ptr& checkable, const Value& commandLine, const ProcessResult& pr)
{
	if (pr.ExitStatus != 0) {
		Process::Arguments parguments = Process::PrepareCommand(commandLine);
		Log(LogWarning, "PluginEventTask")
			<< "Event command for object '" << checkable->GetName() << "' (PID: " << pr.PID
			<< ", arguments: " << Process::PrettyPrintArguments(parguments) << ") terminated with exit code "
			<< pr.ExitStatus << ", output: " << pr.Output;
	}
}
