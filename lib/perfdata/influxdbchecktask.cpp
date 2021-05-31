/* Icinga 2 | (c) 2021 Icinga GmbH | GPLv2+ */

#include "perfdata/influxdbchecktask.hpp"
#include "icinga/host.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "remote/apilistener.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, InfluxdbCheck, &InfluxdbCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

static void ReportInfluxdbCheck(
	const Checkable::Ptr& checkable, const CheckCommand::Ptr& commandObj,
	const CheckResult::Ptr& cr, String output, ServiceState state = ServiceUnknown
)
{
	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		double now = Utility::GetTime();
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = std::move(output);
		pr.ExecutionStart = now;
		pr.ExecutionEnd = now;
		pr.ExitStatus = state;

		Checkable::ExecuteCommandProcessFinishedHandler(commandObj->GetName(), pr);
	} else {
		cr->SetState(state);
		cr->SetOutput(output);
		checkable->ProcessCheckResult(cr);
	}
}

void InfluxdbCheckTask::ScriptFunc(
	const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros
)
{
	ServiceState state = ServiceOK;
	CheckCommand::Ptr commandObj = CheckCommand::ExecuteOverride ? CheckCommand::ExecuteOverride : checkable->GetCheckCommand();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;

	if (MacroResolver::OverrideMacros) {
		resolvers.emplace_back("override", MacroResolver::OverrideMacros);
	}

	if (service) {
		resolvers.emplace_back("service", service);
	}

	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", commandObj);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	String influxdbWriterName = MacroProcessor::ResolveMacros("$influxdbwriter_name$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	String missingQueriesWarning;
	String missingQueriesCritical;
	String missingPendingQueriesWarn;
	String missingPendingQueriesCrit;

	double queriesWarning = MacroProcessor::ResolveMacros("$influxdbwriter_queries_warning$", resolvers, checkable->GetLastCheckResult(),
		&missingQueriesWarning, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	double queriesCritical = MacroProcessor::ResolveMacros("$influxdbwriter_queries_critical$", resolvers, checkable->GetLastCheckResult(),
		&missingQueriesCritical, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	double pendingQueriesWarn = MacroProcessor::ResolveMacros("$influxdbwriter_pending_queries_warning$", resolvers, checkable->GetLastCheckResult(),
		&missingPendingQueriesWarn, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	double pendingQueriesCrit = MacroProcessor::ResolveMacros("$influxdbwriter_pending_queries_critical$", resolvers, checkable->GetLastCheckResult(),
		&missingPendingQueriesCrit, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	/*
	 * Don't execute built-in commands on the master in remote command execution mode
	 * see #2267
	 */
	if (resolvedMacros && !useResolvedMacros) {
		return;
	}

	if (influxdbWriterName.IsEmpty()) {
		ReportInfluxdbCheck(checkable, commandObj, cr, "Attribute 'influxdbwriter_name' must be set.");
		return;
	}

	InfluxdbWriter::Ptr influxdbWriter = InfluxdbWriter::GetByName(influxdbWriterName);

	if (!influxdbWriter) {
		ReportInfluxdbCheck(checkable, commandObj, cr, "InfluxdbWriter '" + influxdbWriterName + "' object does not exist.");
		return;
	}

	if (influxdbWriter->IsPaused()) {
		ReportInfluxdbCheck(checkable, commandObj, cr, "InfluxdbWriter is temporarily disabled on this cluster instance.", state);
		return;
	}

	double queries = influxdbWriter->GetQueryCount(60) / 60.0;
	double pendingQueries = influxdbWriter->GetPendingQueries();
	std::ostringstream msgBuffer;

	msgBuffer << "Queries per second: " << std::fixed << std::setprecision(3) << queries << ".";

	/* Check whether the thresholds have been defined and match. */
	if (missingQueriesCritical.IsEmpty() && queries < queriesCritical) {
		msgBuffer << " " << queries << " queries/s lower than critical threshold (" << queriesCritical << " queries/s).";

		state = ServiceCritical;
	} else if (missingQueriesWarning.IsEmpty() && queries < queriesWarning) {
		msgBuffer << " " << queries << " queries/s lower than warning threshold (" << queriesWarning << " queries/s).";

		state = ServiceWarning;
	}

	if (missingPendingQueriesCrit.IsEmpty() && pendingQueries > pendingQueriesCrit) {
		msgBuffer << " " << pendingQueries << " pending queries greater than critical threshold ("
			<< pendingQueriesCrit << " queries).";

		state = ServiceCritical;
	} else if (missingPendingQueriesWarn.IsEmpty() && pendingQueries > pendingQueriesWarn) {
		msgBuffer << " " << pendingQueries << " pending queries greater than warning threshold ("
			<< pendingQueriesWarn << " queries).";

		if (state == ServiceOK) {
			state = ServiceWarning;
		}
	}

	cr->SetPerformanceData(new Array({
		{ new PerfdataValue("queries", queries, false, "", queriesWarning, queriesCritical) },
		{ new PerfdataValue("queries_1min", influxdbWriter->GetQueryCount(60)) },
		{ new PerfdataValue("queries_5mins", influxdbWriter->GetQueryCount(5 * 60)) },
		{ new PerfdataValue("queries_15mins", influxdbWriter->GetQueryCount(15 * 60)) },
		{ new PerfdataValue("pending_queries", pendingQueries, false, "", pendingQueriesWarn, pendingQueriesCrit) }
	}));

	auto errorMessage = influxdbWriter->GetLastErrorMessage();
	if (!errorMessage.IsEmpty()) {
		ReportInfluxdbCheck(checkable, commandObj, cr, errorMessage, ServiceCritical);
		return;
	}

	ReportInfluxdbCheck(checkable, commandObj, cr, msgBuffer.str(), state);
}
