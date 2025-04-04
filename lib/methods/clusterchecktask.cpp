/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "methods/clusterchecktask.hpp"
#include "remote/apilistener.hpp"
#include "remote/endpoint.hpp"
#include "icinga/cib.hpp"
#include "icinga/service.hpp"
#include "icinga/icingaapplication.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/checkcommand.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/function.hpp"
#include "base/configtype.hpp"
#include <boost/algorithm/string/join.hpp>

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, ClusterCheck, &ClusterCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void ClusterCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
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

	auto perfdataFilter (MacroProcessor::ResolveMacros("$cluster_perfdata$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros));

	if (resolvedMacros && !useResolvedMacros)
		return;

	String commandName = command->GetName();

	ApiListener::Ptr listener = ApiListener::GetInstance();
	if (!listener) {
		String output = "No API listener is configured for this instance.";

		if (Checkable::ExecuteCommandProcessFinishedHandler) {
			double now = Utility::GetTime();
			ProcessResult pr;
			pr.PID = -1;
			pr.ExecutionStart = now;
			pr.ExecutionEnd = now;
			pr.ExitStatus = 126;
			pr.Output = output;
			Checkable::ExecuteCommandProcessFinishedHandler(commandName, pr);
		} else {
			cr->SetOutput(output);
			cr->SetState(ServiceUnknown);
			checkable->ProcessCheckResult(cr);
		}

		return;
	}

	std::pair<Dictionary::Ptr, Dictionary::Ptr> stats = listener->GetStatus();
	Dictionary::Ptr status = stats.first;
	int numConnEndpoints = status->Get("num_conn_endpoints");
	int numNotConnEndpoints = status->Get("num_not_conn_endpoints");

	ServiceState state;
	String output = "Icinga 2 Cluster";

	if (numNotConnEndpoints > 0) {
		output += " Problem: " + Convert::ToString(numNotConnEndpoints) + " endpoints are not connected.";
		output += "\n(" + FormatArray(status->Get("not_conn_endpoints")) + ")";

		state = ServiceCritical;
	} else {
		output += " OK: " + Convert::ToString(numConnEndpoints) + " endpoints are connected.";
		output += "\n(" + FormatArray(status->Get("conn_endpoints")) + ")";

		state = ServiceOK;
	}

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
		/* use feature stats perfdata */
		std::pair<Dictionary::Ptr, Array::Ptr> feature_stats = CIB::GetFeatureStats();
		auto& perfdata (feature_stats.second);
		Utility::FilterPerfdata(perfdata, perfdataFilter);
		cr->SetPerformanceData(perfdata);

		cr->SetCommand(commandName);
		cr->SetState(state);
		cr->SetOutput(output);

		checkable->ProcessCheckResult(cr);
	}
}

String ClusterCheckTask::FormatArray(const Array::Ptr& arr)
{
	bool first = true;
	String str;

	if (arr) {
		ObjectLock olock(arr);
		for (const Value& value : arr) {
			if (first)
				first = false;
			else
				str += ", ";

			str += Convert::ToString(value);
		}
	}

	return str;
}
