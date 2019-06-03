/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "methods/clusterzonechecktask.hpp"
#include "icinga/checkcommand.hpp"
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

	if (!listener) {
		cr->SetOutput("No API listener is configured for this instance.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	CheckCommand::Ptr commandObj = checkable->GetCheckCommand();
	Value raw_command = commandObj->GetCommandLine();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", commandObj);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

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
		cr->SetOutput("Macro 'cluster_zone' must be set.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
		return;
	}

	Zone::Ptr zone = Zone::GetByName(zoneName);

	if (!zone) {
		cr->SetOutput("Zone '" + zoneName + "' does not exist.");
		cr->SetState(ServiceUnknown);
		checkable->ProcessCheckResult(cr);
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

	for (const Endpoint::Ptr& endpoint : zone->GetEndpoints()) {
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

	if (connected) {
		cr->SetState(ServiceOK);
		cr->SetOutput("Zone '" + zoneName + "' is connected. Log lag: " + Utility::FormatDuration(zoneLag));

		/* Check whether the thresholds have been resolved and compare them */
		if (missingLagCritical.IsEmpty() && zoneLag > lagCritical) {
			cr->SetState(ServiceCritical);
			cr->SetOutput("Zone '" + zoneName + "' is connected. Log lag: " + Utility::FormatDuration(zoneLag)
				+ " greater than critical threshold: " + Utility::FormatDuration(lagCritical));
		} else if (missingLagWarning.IsEmpty() && zoneLag > lagWarning) {
			cr->SetState(ServiceWarning);
			cr->SetOutput("Zone '" + zoneName + "' is connected. Log lag: " + Utility::FormatDuration(zoneLag)
				+ " greater than warning threshold: " + Utility::FormatDuration(lagWarning));
		}
	} else {
		cr->SetState(ServiceCritical);
		cr->SetOutput("Zone '" + zoneName + "' is not connected. Log lag: " + Utility::FormatDuration(zoneLag));
	}

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
