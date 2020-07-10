/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "methods/executeactiontask.hpp"
#include "icinga/pluginutility.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/clusterevents.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "remote/apilistener.hpp"
#include "base/configtype.hpp"
#include "base/logger.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/process.hpp"
#include "base/convert.hpp"

using namespace icinga;

void ExecuteActionTask::ProcessFinishedHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr, const Value& commandLine, const ProcessResult& pr)
{
	Checkable::CurrentConcurrentChecks.fetch_sub(1);
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

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	Dictionary::Ptr executedParams = new Dictionary();
	executedParams->Set("host", host->GetName());
	if (service)
		executedParams->Set("service", service->GetShortName());

	/* TODO set the execution UUID */
	/*executedParams->Set("execution", uuid);*/

	executedParams->Set("check_result", cr);

	/* FIXME command endpoint was overwrite by macro? */
	Endpoint::Ptr commandEndpoint = checkable->GetCommandEndpoint();
	bool local = !commandEndpoint || commandEndpoint == Endpoint::GetLocalEndpoint();
	if (local) {
		MessageOrigin::Ptr origin = new MessageOrigin();
		ClusterEvents::ExecutedCommandAPIHandler(origin, executedParams);
	} else {
		ApiListener::Ptr listener = ApiListener::GetInstance();

		if (listener) {
			Dictionary::Ptr executedMessage = new Dictionary();
			executedMessage->Set("jsonrpc", "2.0");
			executedMessage->Set("method", "event::ExecutedCommand");
			executedMessage->Set("params", executedParams);

			listener->SyncSendMessage(commandEndpoint, executedMessage);
		} else {
			Log(LogCritical, "ExecuteActionTask") << "Api listener not found";
		}
	}
}
