/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "methods/clusterzonechecktask.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/envresolver.hpp"
#include "icinga/macroprocessor.hpp"
#include "remote/apilistener.hpp"
#include "remote/endpoint.hpp"
#include "remote/zone.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, ClusterZoneCheck, &ClusterZoneCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void ClusterZoneCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	REQUIRE_NOT_NULL(checkable);
	REQUIRE_NOT_NULL(cr);

	ApiListener::Ptr listener = ApiListener::GetInstance();
	CheckCommand::Ptr command = CheckCommand::ExecuteOverride ? CheckCommand::ExecuteOverride : checkable->GetCheckCommand();
	String commandName = command->GetName();

	if (!listener) {
		String output = "No API listener is configured for this instance.";
		ServiceState state = ServiceUnknown;

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
			cr->SetCommand(commandName);
			cr->SetOutput(output);
			cr->SetState(state);
			checkable->ProcessCheckResult(cr);
		}

		return;
	}

	Value raw_command = command->GetCommandLine();

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
	resolvers.emplace_back("env", new EnvResolver(), false);

	String zoneName = MacroProcessor::ResolveMacros("$cluster_zone$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	String missingLagWarning;
	String missingLagCritical;

	double lagWarning = MacroProcessor::ResolveMacros("$cluster_lag_warning$", resolvers, checkable->GetLastCheckResult(),
		&missingLagWarning, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	double lagCritical = MacroProcessor::ResolveMacros("$cluster_lag_critical$", resolvers, checkable->GetLastCheckResult(),
		&missingLagCritical, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	if (resolvedMacros && !useResolvedMacros)
		return;

	if (zoneName.IsEmpty()) {
		String output = "Macro 'cluster_zone' must be set.";
		ServiceState state = ServiceUnknown;

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
			cr->SetCommand(commandName);
			cr->SetOutput(output);
			cr->SetState(state);
			checkable->ProcessCheckResult(cr);
		}

		return;
	}

	Zone::Ptr zone = Zone::GetByName(zoneName);

	if (!zone) {
		String output = "Zone '" + zoneName + "' does not exist.";
		ServiceState state = ServiceUnknown;

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
			cr->SetCommand(commandName);
			cr->SetOutput(output);
			cr->SetState(state);
			checkable->ProcessCheckResult(cr);
		}
		return;
	}

	bool connected = false;
	double zoneLag = 0;

	double lastMessageSent = 0;
	double lastMessageReceived = 0;
	double messagesSentPerSecond = 0;
	double messagesReceivedPerSecond = 0;
	double bytesSentPerSecond = 0;
	double bytesReceivedPerSecond = 0;

	{
		auto endpoints (zone->GetEndpoints());

		for (const Endpoint::Ptr& endpoint : endpoints) {
			if (endpoint->GetConnected())
				connected = true;

			double eplag = ApiListener::CalculateZoneLag(endpoint);

			if (eplag > 0 && eplag > zoneLag)
				zoneLag = eplag;

			if (endpoint->GetLastMessageSent() > lastMessageSent)
				lastMessageSent = endpoint->GetLastMessageSent();

			if (endpoint->GetLastMessageReceived() > lastMessageReceived)
				lastMessageReceived = endpoint->GetLastMessageReceived();

			messagesSentPerSecond += endpoint->GetMessagesSentPerSecond();
			messagesReceivedPerSecond += endpoint->GetMessagesReceivedPerSecond();
			bytesSentPerSecond += endpoint->GetBytesSentPerSecond();
			bytesReceivedPerSecond += endpoint->GetBytesReceivedPerSecond();
		}

		if (!connected && endpoints.size() == 1u && *endpoints.begin() == Endpoint::GetLocalEndpoint()) {
			connected = true;
		}
	}

	ServiceState state;
	String output;

	if (connected) {
		state = ServiceOK;
		output = "Zone '" + zoneName + "' is connected. Log lag: " + Utility::FormatDuration(zoneLag);

		/* Check whether the thresholds have been resolved and compare them */
		if (missingLagCritical.IsEmpty() && zoneLag > lagCritical) {
			state = ServiceCritical;
			output = "Zone '" + zoneName + "' is connected. Log lag: " + Utility::FormatDuration(zoneLag)
				+ " greater than critical threshold: " + Utility::FormatDuration(lagCritical);
		} else if (missingLagWarning.IsEmpty() && zoneLag > lagWarning) {
			state = ServiceWarning;
			output = "Zone '" + zoneName + "' is connected. Log lag: " + Utility::FormatDuration(zoneLag)
				+ " greater than warning threshold: " + Utility::FormatDuration(lagWarning);
		}
	} else {
		state = ServiceCritical;
		output = "Zone '" + zoneName + "' is not connected. Log lag: " + Utility::FormatDuration(zoneLag);
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
		cr->SetCommand(commandName);
		cr->SetState(state);
		cr->SetOutput(output);
		cr->SetPerformanceData(new Array({
			new PerfdataValue("slave_lag", zoneLag, false, "s", lagWarning, lagCritical),
			new PerfdataValue("last_messages_sent", lastMessageSent),
			new PerfdataValue("last_messages_received", lastMessageReceived),
			new PerfdataValue("sum_messages_sent_per_second", messagesSentPerSecond),
			new PerfdataValue("sum_messages_received_per_second", messagesReceivedPerSecond),
			new PerfdataValue("sum_bytes_sent_per_second", bytesSentPerSecond),
			new PerfdataValue("sum_bytes_received_per_second", bytesReceivedPerSecond)
		}));

		checkable->ProcessCheckResult(cr);
	}
}
