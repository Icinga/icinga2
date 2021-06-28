/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "db_ido/idochecktask.hpp"
#include "icinga/host.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "remote/apilistener.hpp"
#include "remote/endpoint.hpp"
#include "remote/zone.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/configtype.hpp"
#include "base/convert.hpp"

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, IdoCheck, &IdoCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

void IdoCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	ServiceState state;
	CheckCommand::Ptr commandObj = CheckCommand::ExecuteOverride ? CheckCommand::ExecuteOverride : checkable->GetCheckCommand();
	Value raw_command = commandObj->GetCommandLine();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (MacroResolver::OverrideMacros)
		resolvers.emplace_back("override", MacroResolver::OverrideMacros);

	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", commandObj);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	String idoType = MacroProcessor::ResolveMacros("$ido_type$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	String idoName = MacroProcessor::ResolveMacros("$ido_name$", resolvers, checkable->GetLastCheckResult(),
		nullptr, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	String missingQueriesWarning;
	String missingQueriesCritical;
	String missingPendingQueriesWarning;
	String missingPendingQueriesCritical;

	double queriesWarning = MacroProcessor::ResolveMacros("$ido_queries_warning$", resolvers, checkable->GetLastCheckResult(),
	    &missingQueriesWarning, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	double queriesCritical = MacroProcessor::ResolveMacros("$ido_queries_critical$", resolvers, checkable->GetLastCheckResult(),
	    &missingQueriesCritical, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	double pendingQueriesWarning = MacroProcessor::ResolveMacros("$ido_pending_queries_warning$", resolvers, checkable->GetLastCheckResult(),
	    &missingPendingQueriesWarning, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	double pendingQueriesCritical = MacroProcessor::ResolveMacros("$ido_pending_queries_critical$", resolvers, checkable->GetLastCheckResult(),
	    &missingPendingQueriesCritical, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);

	if (resolvedMacros && !useResolvedMacros)
		return;

	if (idoType.IsEmpty()) {
		String output = "Attribute 'ido_type' must be set.";
		state = ServiceUnknown;

		if (Checkable::ExecuteCommandProcessFinishedHandler) {
			double now = Utility::GetTime();
			ProcessResult pr;
			pr.PID = -1;
			pr.Output = output;
			pr.ExecutionStart = now;
			pr.ExecutionEnd = now;
			pr.ExitStatus = state;

			Checkable::ExecuteCommandProcessFinishedHandler(commandObj->GetName(), pr);
		} else {
			cr->SetState(state);
			cr->SetOutput(output);
			checkable->ProcessCheckResult(cr);
		}
		return;
	}

	if (idoName.IsEmpty()) {
		String output = "Attribute 'ido_name' must be set.";
		state = ServiceUnknown;

		if (Checkable::ExecuteCommandProcessFinishedHandler) {
			double now = Utility::GetTime();
			ProcessResult pr;
			pr.PID = -1;
			pr.Output = output;
			pr.ExecutionStart = now;
			pr.ExecutionEnd = now;
			pr.ExitStatus = state;

			Checkable::ExecuteCommandProcessFinishedHandler(commandObj->GetName(), pr);
		} else {
			cr->SetState(state);
			cr->SetOutput(output);
			checkable->ProcessCheckResult(cr);
		}
		return;
	}

	Type::Ptr type = Type::GetByName(idoType);

	if (!type || !DbConnection::TypeInstance->IsAssignableFrom(type)) {
		String output = "DB IDO type '" + idoType + "' is invalid.";
		state = ServiceUnknown;

		if (Checkable::ExecuteCommandProcessFinishedHandler) {
			double now = Utility::GetTime();
			ProcessResult pr;
			pr.PID = -1;
			pr.Output = output;
			pr.ExecutionStart = now;
			pr.ExecutionEnd = now;
			pr.ExitStatus = state;

			Checkable::ExecuteCommandProcessFinishedHandler(commandObj->GetName(), pr);
		} else {
			cr->SetState(state);
			cr->SetOutput(output);
			checkable->ProcessCheckResult(cr);
		}
		return;
	}

	auto *dtype = dynamic_cast<ConfigType *>(type.get());
	VERIFY(dtype);

	DbConnection::Ptr conn = static_pointer_cast<DbConnection>(dtype->GetObject(idoName));

	if (!conn) {
		String output = "DB IDO connection '" + idoName + "' does not exist.";
		state = ServiceUnknown;

		if (Checkable::ExecuteCommandProcessFinishedHandler) {
			double now = Utility::GetTime();
			ProcessResult pr;
			pr.PID = -1;
			pr.Output = output;
			pr.ExecutionStart = now;
			pr.ExecutionEnd = now;
			pr.ExitStatus = state;

			Checkable::ExecuteCommandProcessFinishedHandler(commandObj->GetName(), pr);
		} else {
			cr->SetState(state);
			cr->SetOutput(output);
			checkable->ProcessCheckResult(cr);
		}
		return;
	}

	double qps = conn->GetQueryCount(60) / 60.0;

	if (conn->IsPaused()) {
		String output = "DB IDO connection is temporarily disabled on this cluster instance.";
		state = ServiceOK;

		if (Checkable::ExecuteCommandProcessFinishedHandler) {
			double now = Utility::GetTime();
			ProcessResult pr;
			pr.PID = -1;
			pr.Output = output;
			pr.ExecutionStart = now;
			pr.ExecutionEnd = now;
			pr.ExitStatus = state;

			Checkable::ExecuteCommandProcessFinishedHandler(commandObj->GetName(), pr);
		} else {
			cr->SetState(state);
			cr->SetOutput(output);
			checkable->ProcessCheckResult(cr);
		}
		return;
	}

	double pendingQueries = conn->GetPendingQueryCount();

	if (!conn->GetConnected()) {
		String output;
		if (conn->GetShouldConnect()) {
			output ="Could not connect to the database server.";
			state = ServiceCritical;
		} else {
			output = "Not currently enabled: Another cluster instance is responsible for the IDO database.";
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

			Checkable::ExecuteCommandProcessFinishedHandler(commandObj->GetName(), pr);
		} else {
			cr->SetState(state);
			cr->SetOutput(output);
			checkable->ProcessCheckResult(cr);
		}
		return;
	}

	/* Schema versions. */
	String schema_version = conn->GetSchemaVersion();
	std::ostringstream msgbuf;

	if (Utility::CompareVersion(IDO_CURRENT_SCHEMA_VERSION, schema_version) < 0) {
		msgbuf << "Outdated schema version: '" << schema_version << "'. Latest version: '"
		    << IDO_CURRENT_SCHEMA_VERSION << "'."
		    << " Queries per second: " << std::fixed << std::setprecision(3) << qps
		    << " Pending queries: " << std::fixed << std::setprecision(3) << pendingQueries << ".";

		state = ServiceWarning;
	} else {
		msgbuf << "Connected to the database server (Schema version: '" << schema_version << "')."
		    << " Queries per second: " << std::fixed << std::setprecision(3) << qps
		    << " Pending queries: " << std::fixed << std::setprecision(3) << pendingQueries << ".";

		state = ServiceOK;
	}

	if (conn->GetEnableHa()) {
		double failoverTs = conn->GetLastFailover();

		msgbuf << " Last failover: " << Utility::FormatDateTime("%Y-%m-%d %H:%M:%S %z", failoverTs) << ".";
	}

	/* Check whether the thresholds have been defined and match. */
	if (missingQueriesCritical.IsEmpty() && qps < queriesCritical) {
		msgbuf << " " << qps << " queries/s lower than critical threshold (" << queriesCritical << " queries/s).";

		state= ServiceCritical;
	} else if (missingQueriesWarning.IsEmpty() && qps < queriesWarning) {
		msgbuf << " " << qps << " queries/s lower than warning threshold (" << queriesWarning << " queries/s).";

		state = ServiceWarning;
	}

	if (missingPendingQueriesCritical.IsEmpty() && pendingQueries > pendingQueriesCritical) {
		msgbuf << " " << pendingQueries << " pending queries greater than critical threshold ("
		    << pendingQueriesCritical << " queries).";

		state = ServiceCritical;
	} else if (missingPendingQueriesWarning.IsEmpty() && pendingQueries > pendingQueriesWarning) {
		msgbuf << " " << pendingQueries << " pending queries greater than warning threshold ("
		    << pendingQueriesWarning << " queries).";

		state = ServiceWarning;
	}

	String output = msgbuf.str();

	if (Checkable::ExecuteCommandProcessFinishedHandler) {
		double now = Utility::GetTime();
		ProcessResult pr;
		pr.PID = -1;
		pr.Output = output;
		pr.ExecutionStart = now;
		pr.ExecutionEnd = now;
		pr.ExitStatus = state;

		Checkable::ExecuteCommandProcessFinishedHandler(commandObj->GetName(), pr);
	} else {
		cr->SetState(state);
		cr->SetOutput(output);

		cr->SetPerformanceData(new Array({
			{ new PerfdataValue("queries", qps, false, "", queriesWarning, queriesCritical) },
			{ new PerfdataValue("queries_1min", conn->GetQueryCount(60)) },
			{ new PerfdataValue("queries_5mins", conn->GetQueryCount(5 * 60)) },
			{ new PerfdataValue("queries_15mins", conn->GetQueryCount(15 * 60)) },
			{ new PerfdataValue("pending_queries", pendingQueries, false, "", pendingQueriesWarning, pendingQueriesCritical) }
		}));

		checkable->ProcessCheckResult(cr);
	}
}
