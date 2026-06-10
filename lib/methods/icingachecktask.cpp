// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "methods/icingachecktask.hpp"
#include "icinga/cib.hpp"
#include "icinga/service.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/clusterevents.hpp"
#include "icinga/checkable.hpp"
#include "remote/apilistener.hpp"
#include "base/application.hpp"
#include "base/objectlock.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/function.hpp"
#include "base/configtype.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, IcingaCheck, &IcingaCheckTask::ScriptFunc, "checkable:cr:producer:resolvedMacros:useResolvedMacros");

void IcingaCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const WaitGroup::Ptr& producer, const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
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

	String missingIcingaMinVersion;

	String icingaMinVersion = MacroProcessor::ResolveMacros("$icinga_min_version$", resolvers, checkable->GetLastCheckResult(),
		&missingIcingaMinVersion, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	if (resolvedMacros && !useResolvedMacros)
		return;

	double interval = Utility::GetTime() - Application::GetStartTime();

	if (interval > 60)
		interval = 60;

	/* use feature stats perfdata */
	std::pair<Dictionary::Ptr, Array::Ptr> feature_stats = CIB::GetFeatureStats();

	Array::Ptr perfdata = feature_stats.second;

	perfdata->Add(new PerfdataValue("active_host_checks", CIB::GetActiveHostChecksStatistics(interval) / interval));
	perfdata->Add(new PerfdataValue("passive_host_checks", CIB::GetPassiveHostChecksStatistics(interval) / interval));
	perfdata->Add(new PerfdataValue("active_host_checks_1min", CIB::GetActiveHostChecksStatistics(60)));
	perfdata->Add(new PerfdataValue("passive_host_checks_1min", CIB::GetPassiveHostChecksStatistics(60)));
	perfdata->Add(new PerfdataValue("active_host_checks_5min", CIB::GetActiveHostChecksStatistics(60 * 5)));
	perfdata->Add(new PerfdataValue("passive_host_checks_5min", CIB::GetPassiveHostChecksStatistics(60 * 5)));
	perfdata->Add(new PerfdataValue("active_host_checks_15min", CIB::GetActiveHostChecksStatistics(60 * 15)));
	perfdata->Add(new PerfdataValue("passive_host_checks_15min", CIB::GetPassiveHostChecksStatistics(60 * 15)));

	perfdata->Add(new PerfdataValue("active_service_checks", CIB::GetActiveServiceChecksStatistics(interval) / interval));
	perfdata->Add(new PerfdataValue("passive_service_checks", CIB::GetPassiveServiceChecksStatistics(interval) / interval));
	perfdata->Add(new PerfdataValue("active_service_checks_1min", CIB::GetActiveServiceChecksStatistics(60)));
	perfdata->Add(new PerfdataValue("passive_service_checks_1min", CIB::GetPassiveServiceChecksStatistics(60)));
	perfdata->Add(new PerfdataValue("active_service_checks_5min", CIB::GetActiveServiceChecksStatistics(60 * 5)));
	perfdata->Add(new PerfdataValue("passive_service_checks_5min", CIB::GetPassiveServiceChecksStatistics(60 * 5)));
	perfdata->Add(new PerfdataValue("active_service_checks_15min", CIB::GetActiveServiceChecksStatistics(60 * 15)));
	perfdata->Add(new PerfdataValue("passive_service_checks_15min", CIB::GetPassiveServiceChecksStatistics(60 * 15)));

	perfdata->Add(new PerfdataValue("current_pending_callbacks", Application::GetTP().GetPending()));
	perfdata->Add(new PerfdataValue("current_concurrent_checks", Checkable::CurrentConcurrentChecks.load()));
	perfdata->Add(new PerfdataValue("remote_check_queue", ClusterEvents::GetCheckRequestQueueSize()));

	CheckableCheckStatistics scs = CIB::CalculateServiceCheckStats();

	perfdata->Add(new PerfdataValue("min_latency", scs.min_latency));
	perfdata->Add(new PerfdataValue("max_latency", scs.max_latency));
	perfdata->Add(new PerfdataValue("avg_latency", scs.avg_latency));
	perfdata->Add(new PerfdataValue("min_execution_time", scs.min_execution_time));
	perfdata->Add(new PerfdataValue("max_execution_time", scs.max_execution_time));
	perfdata->Add(new PerfdataValue("avg_execution_time", scs.avg_execution_time));

	ServiceStatistics ss = CIB::CalculateServiceStats();

	perfdata->Add(new PerfdataValue("num_services_ok", ss.services_ok));
	perfdata->Add(new PerfdataValue("num_services_warning", ss.services_warning));
	perfdata->Add(new PerfdataValue("num_services_critical", ss.services_critical));
	perfdata->Add(new PerfdataValue("num_services_unknown", ss.services_unknown));
	perfdata->Add(new PerfdataValue("num_services_pending", ss.services_pending));
	perfdata->Add(new PerfdataValue("num_services_unreachable", ss.services_unreachable));
	perfdata->Add(new PerfdataValue("num_services_flapping", ss.services_flapping));
	perfdata->Add(new PerfdataValue("num_services_in_downtime", ss.services_in_downtime));
	perfdata->Add(new PerfdataValue("num_services_acknowledged", ss.services_acknowledged));
	perfdata->Add(new PerfdataValue("num_services_handled", ss.services_handled));
	perfdata->Add(new PerfdataValue("num_services_problem", ss.services_problem));

	double uptime = Application::GetUptime();
	perfdata->Add(new PerfdataValue("uptime", uptime));

	HostStatistics hs = CIB::CalculateHostStats();

	perfdata->Add(new PerfdataValue("num_hosts_up", hs.hosts_up));
	perfdata->Add(new PerfdataValue("num_hosts_down", hs.hosts_down));
	perfdata->Add(new PerfdataValue("num_hosts_pending", hs.hosts_pending));
	perfdata->Add(new PerfdataValue("num_hosts_unreachable", hs.hosts_unreachable));
	perfdata->Add(new PerfdataValue("num_hosts_flapping", hs.hosts_flapping));
	perfdata->Add(new PerfdataValue("num_hosts_in_downtime", hs.hosts_in_downtime));
	perfdata->Add(new PerfdataValue("num_hosts_acknowledged", hs.hosts_acknowledged));
	perfdata->Add(new PerfdataValue("num_hosts_handled", hs.hosts_handled));
	perfdata->Add(new PerfdataValue("num_hosts_problem", hs.hosts_problem));

	std::vector<Endpoint::Ptr> endpoints = ConfigType::GetObjectsByType<Endpoint>();

	double lastMessageSent = 0;
	double lastMessageReceived = 0;
	uint_fast64_t pendingOutgoingMessages = 0;
	double messagesSentPerSecond = 0;
	double messagesReceivedPerSecond = 0;
	double bytesSentPerSecond = 0;
	double bytesReceivedPerSecond = 0;

	for (const Endpoint::Ptr& endpoint : endpoints)
	{
		if (endpoint->GetLastMessageSent() > lastMessageSent)
			lastMessageSent = endpoint->GetLastMessageSent();

		if (endpoint->GetLastMessageReceived() > lastMessageReceived)
			lastMessageReceived = endpoint->GetLastMessageReceived();

		pendingOutgoingMessages += endpoint->GetPendingOutgoingMessages();
		messagesSentPerSecond += endpoint->GetMessagesSentPerSecond();
		messagesReceivedPerSecond += endpoint->GetMessagesReceivedPerSecond();
		bytesSentPerSecond += endpoint->GetBytesSentPerSecond();
		bytesReceivedPerSecond += endpoint->GetBytesReceivedPerSecond();
	}

	perfdata->Add(new PerfdataValue("last_messages_sent", lastMessageSent));
	perfdata->Add(new PerfdataValue("last_messages_received", lastMessageReceived));
	perfdata->Add(new PerfdataValue("sum_pending_outgoing_messages", pendingOutgoingMessages));
	perfdata->Add(new PerfdataValue("sum_messages_sent_per_second", messagesSentPerSecond));
	perfdata->Add(new PerfdataValue("sum_messages_received_per_second", messagesReceivedPerSecond));
	perfdata->Add(new PerfdataValue("sum_bytes_sent_per_second", bytesSentPerSecond));
	perfdata->Add(new PerfdataValue("sum_bytes_received_per_second", bytesReceivedPerSecond));

	cr->SetPerformanceData(perfdata);
	ServiceState state = ServiceOK;

	String appVersion = Application::GetAppVersion();

	String output = "Icinga 2 has been running for " + Utility::FormatDuration(uptime) +
		". Version: " + appVersion;

	/* Indicate a warning if the last reload failed. */
	double lastReloadFailed = Application::GetLastReloadFailed();

	if (lastReloadFailed > 0) {
		output += "; Last reload attempt failed at " + Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", lastReloadFailed);
		state =ServiceWarning;
	}

	/* Indicate a warning when the last synced config caused a stage validation error. */
	ApiListener::Ptr listener = ApiListener::GetInstance();

	if (listener) {
		Dictionary::Ptr validationResult = listener->GetLastFailedZonesStageValidation();

		if (validationResult) {
			output += "; Last zone sync stage validation failed at "
				+ Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", validationResult->Get("ts"));

			state = ServiceWarning;
		}
	}

	String parsedAppVersion = Utility::ParseVersion(appVersion);

	/* Return an error if the version is less than specified (optional). */
	if (missingIcingaMinVersion.IsEmpty() && !icingaMinVersion.IsEmpty() && Utility::CompareVersion(icingaMinVersion, parsedAppVersion) < 0) {
		output += "; Minimum version " + icingaMinVersion + " is not installed.";
		state = ServiceCritical;
	}

	String commandName = command->GetName();

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
		cr->SetState(state);
		cr->SetOutput(output);
		cr->SetCommand(commandName);

		checkable->ProcessCheckResult(cr, producer);
	}
}
