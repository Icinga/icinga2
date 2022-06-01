/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#include "icingadb/icingadbchecktask.hpp"
#include "icinga/host.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "remote/apilistener.hpp"
#include "remote/endpoint.hpp"
#include "remote/zone.hpp"
#include "base/function.hpp"
#include "base/json.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/configtype.hpp"
#include "base/convert.hpp"
#include <utility>

using namespace icinga;

REGISTER_FUNCTION_NONCONST(Internal, IcingadbCheck, &IcingadbCheckTask::ScriptFunc, "checkable:cr:resolvedMacros:useResolvedMacros");

static void ReportIcingadbCheck(
	const Checkable::Ptr& checkable, const CheckCommand::Ptr& commandObj,
	const CheckResult::Ptr& cr, String output, ServiceState state)
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

static inline
double GetXMessageTs(const Array::Ptr& xMessage)
{
	return Convert::ToLong(String(xMessage->Get(0)).Split("-")[0]) / 1000.0;
}

void IcingadbCheckTask::ScriptFunc(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr,
	const Dictionary::Ptr& resolvedMacros, bool useResolvedMacros)
{
	CheckCommand::Ptr commandObj = CheckCommand::ExecuteOverride ? CheckCommand::ExecuteOverride : checkable->GetCheckCommand();

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	String silenceMissingMacroWarning;

	if (MacroResolver::OverrideMacros)
		resolvers.emplace_back("override", MacroResolver::OverrideMacros);

	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("command", commandObj);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	auto resolve ([&resolvers, &checkable, &silenceMissingMacroWarning, &resolvedMacros, useResolvedMacros](const String& macro) {
		return MacroProcessor::ResolveMacros(macro, resolvers, checkable->GetLastCheckResult(),
			&silenceMissingMacroWarning, MacroProcessor::EscapeCallback(), resolvedMacros, useResolvedMacros);
	});

	struct Thresholds
	{
		Value Warning, Critical;
	};

	auto resolveThresholds ([&resolve](const String& wmacro, const String& cmacro) {
		return Thresholds{resolve(wmacro), resolve(cmacro)};
	});

	String icingadbName = resolve("$icingadb_name$");

	auto downForThresholds (resolveThresholds("$icingadb_downfor_warning$", "$icingadb_downfor_critical$"));
	auto heartbeatThresholds (resolveThresholds("$icingadb_heartbeat_warning$", "$icingadb_heartbeat_critical$"));
	auto idleForThresholds (resolveThresholds("$icingadb_idlefor_warning$", "$icingadb_idlefor_critical$"));
	auto historyBacklogThresholds (resolveThresholds("$icingadb_history_backlog_warning$", "$icingadb_history_backlog_critical$"));
	auto queriesThresholds (resolveThresholds("$icingadb_queries_warning$", "$icingadb_queries_critical$"));
	auto pendingQueriesThresholds (resolveThresholds("$icingadb_pending_queries_warning$", "$icingadb_pending_queries_critical$"));
	auto syncAgoThresholds (resolveThresholds("$icingadb_syncago_warning$", "$icingadb_syncago_critical$"));
	auto syncTookThresholds (resolveThresholds("$icingadb_synctook_warning$", "$icingadb_synctook_critical$"));
	auto dumpAgoThresholds (resolveThresholds("$icingadb_dumpago_warning$", "$icingadb_dumpago_critical$"));
	auto dumpTookThresholds (resolveThresholds("$icingadb_dumptook_warning$", "$icingadb_dumptook_critical$"));

	std::map<String, Thresholds> thresholdsByOp;

	const char * const icingadbKnownStats[] = {
		"sync_config", "sync_state", "sync_history", "sync_overdue", "cleanup_history"
	};

	for (auto metric : icingadbKnownStats) {
		thresholdsByOp.emplace(metric, resolveThresholds(
			String("$icingadb_") + metric + "_warning$",
			String("$icingadb_") + metric + "_critical$"
		));
	}

	if (resolvedMacros && !useResolvedMacros)
		return;

	if (icingadbName.IsEmpty()) {
		ReportIcingadbCheck(checkable, commandObj, cr, "Attribute 'icingadb_name' must be set.", ServiceUnknown);
		return;
	}

	auto conn (IcingaDB::GetByName(icingadbName));

	if (!conn) {
		ReportIcingadbCheck(checkable, commandObj, cr, "Icinga DB connection '" + icingadbName + "' does not exist.", ServiceUnknown);
		return;
	}

	auto redis (conn->GetConnection());

	if (!redis->GetConnected()) {
		ReportIcingadbCheck(checkable, commandObj, cr, "Could not connect to Redis.", ServiceCritical);
		return;
	}

	Array::Ptr xReadHeartbeat, xReadStats, xReadHistory;

	try {
		auto replies (redis->GetResultsOfQueries(
			{
				{"XREAD", "STREAMS", "icingadb:telemetry:heartbeat", "0-0"},
				{"XREAD", "STREAMS", "icingadb:telemetry:stats", "0-0"},
				{
					"XREAD", "COUNT", "1", "STREAMS",
					"icinga:history:stream:acknowledgement", "icinga:history:stream:comment",
					"icinga:history:stream:downtime", "icinga:history:stream:flapping",
					"icinga:history:stream:notification", "icinga:history:stream:state",
					"0-0", "0-0", "0-0", "0-0", "0-0", "0-0"
				}
			},
			RedisConnection::QueryPriority::Heartbeat
		));

		xReadHeartbeat = std::move(replies.at(0));
		xReadStats = std::move(replies.at(1));
		xReadHistory = std::move(replies.at(2));
	} catch (const std::exception& ex) {
		ReportIcingadbCheck(checkable, commandObj, cr, String("Could not read XREAD responses from Redis: ") + ex.what(), ServiceCritical);
		return;
	}

	if (!xReadHeartbeat) {
		ReportIcingadbCheck(
			checkable, commandObj, cr,
			"The Icinga DB daemon seems to have never run. (Missing heartbeat)",
			ServiceCritical
		);

		return;
	}

	auto dumpWhen (conn->GetLastdumpEnd());
	auto dumpTook (conn->GetLastdumpTook());
	Array::Ptr heartbeatMessage = Array::Ptr(Array::Ptr(xReadHeartbeat->Get(0))->Get(1))->Get(0);
	auto heartbeatTime (GetXMessageTs(heartbeatMessage));
	std::map<String, String> heartbeatData;

	IcingaDB::AddKvsToMap(heartbeatMessage->Get(1), heartbeatData);

	String version = heartbeatData.at("meta:version");
	Dictionary::Ptr goMetricsByCumulativity (JsonDecode(heartbeatData.at("go:metrics")));
	String dbErr (heartbeatData.at("db:err"));
	auto ourHeartbeatTs (Convert::ToLong(heartbeatData.at("heartbeat:last-ts")) / 1000.0);
	bool weResponsible = Convert::ToLong(heartbeatData.at("ha:we-responsible"));
	auto weResponsibleTs (Convert::ToLong(heartbeatData.at("ha:we-responsible-ts")) / 1000.0);
	bool otherResponsible = Convert::ToLong(heartbeatData.at("ha:other-responsible"));
	auto syncWhen (Convert::ToLong(heartbeatData.at("sync:when")) / 1000.0);
	auto syncTook (Convert::ToDouble(heartbeatData.at("sync:took")) / 1000);

	auto now (Utility::GetTime());
	auto downFor (now - heartbeatTime);
	auto responsibleFor (now - weResponsibleTs);
	auto idleFor ((weResponsible ? -1 : 1) * responsibleFor);
	auto heartbeatLag (now - ourHeartbeatTs);
	auto syncAgo (now - syncWhen);
	auto dumpAgo (now - dumpWhen);
	double historyBacklog = 0;

	if (xReadHistory) {
		double minTs = 0;
		ObjectLock lock (xReadHistory);

		for (Array::Ptr stream : xReadHistory) {
			auto ts (GetXMessageTs(Array::Ptr(stream->Get(1))->Get(0)));

			if (minTs == 0 || ts < minTs) {
				minTs = ts;
			}
		}

		if (minTs > 0) {
			historyBacklog = now - minTs;
		}
	}

	Array::Ptr perfdata = new Array();
	std::map<String, RingBuffer> statsPerOp;

	for (auto metric : icingadbKnownStats) {
		statsPerOp.emplace(std::piecewise_construct, std::forward_as_tuple(metric), std::forward_as_tuple(15 * 60));
	}

	if (xReadStats) {
		Array::Ptr messages = Array::Ptr(xReadStats->Get(0))->Get(1);
		ObjectLock lock (messages);

		for (Array::Ptr message : messages) {
			auto ts (GetXMessageTs(message));
			std::map<String, String> opsPerSec;

			IcingaDB::AddKvsToMap(message->Get(1), opsPerSec);

			for (auto& kv : opsPerSec) {
				auto buf (statsPerOp.find(kv.first));

				if (buf == statsPerOp.end()) {
					buf = statsPerOp.emplace(
						std::piecewise_construct,
						std::forward_as_tuple(kv.first), std::forward_as_tuple(15 * 60)
					).first;
				}

				buf->second.InsertValue(ts, Convert::ToLong(kv.second));
			}
		}
	}

	ServiceState state = ServiceOK;
	std::ostringstream msgbuf;
	double qps = redis->GetQueryCount(60) / 60.0;
	double pendingQueries = redis->GetPendingQueryCount();

	auto checkLower ([&state, &msgbuf](double value, const Thresholds& thresholds) {
		if (!thresholds.Critical.IsEmpty() && value < (double)thresholds.Critical) {
			msgbuf << ", lower than CRITICAL threshold (" << thresholds.Critical << ")";
			state = ServiceCritical;
		} else if (!thresholds.Warning.IsEmpty() && value < (double)thresholds.Warning) {
			msgbuf << ", lower than WARNING threshold (" << thresholds.Warning << ")";

			if (state == ServiceOK) {
				state = ServiceWarning;
			}
		}
	});

	auto checkGreater ([&state, &msgbuf](double value, const Thresholds& thresholds) {
		if (!thresholds.Critical.IsEmpty() && value > (double)thresholds.Critical) {
			msgbuf << ", greater than CRITICAL threshold (" << thresholds.Critical << ")";
			state = ServiceCritical;
		} else if (!thresholds.Warning.IsEmpty() && value > (double)thresholds.Warning) {
			msgbuf << ", greater than WARNING threshold (" << thresholds.Warning << ")";

			if (state == ServiceOK) {
				state = ServiceWarning;
			}
		}
	});

	msgbuf << std::fixed << std::setprecision(3)
		<< "Icinga 2\n--------\n"
		<< "\n* Connected to Redis"
		<< "\n* Queries per second: " << qps;

	checkLower(qps, queriesThresholds);

	msgbuf << "\n* Pending queries: " << pendingQueries;
	checkGreater(pendingQueries, pendingQueriesThresholds);

	msgbuf << "\n* Last dump: ";

	if (dumpWhen) {
		msgbuf << dumpAgo << " seconds ago";
		perfdata->Add(new PerfdataValue("dump_ago", dumpAgo, false, "seconds", dumpAgoThresholds.Warning, dumpAgoThresholds.Critical, 0));
	} else {
		msgbuf << "never";
	}

	checkGreater(dumpAgo, dumpAgoThresholds);

	if (dumpWhen) {
		msgbuf << "\n* Last dump took: " << dumpTook << " seconds";
		checkGreater(dumpTook, dumpTookThresholds);
		perfdata->Add(new PerfdataValue("dump_took", dumpTook, false, "seconds", dumpTookThresholds.Warning, dumpTookThresholds.Critical, 0));
	}

	msgbuf << "\n\nIcinga DB daemon\n----------------\n"
		<< "\n* Version: " << version;

	if (!dbErr.IsEmpty()) {
		msgbuf << "\n* Database ERROR: " << dbErr;
		state = ServiceCritical;
	}

	msgbuf << "\n* Last seen: " << downFor << " seconds ago";
	checkGreater(downFor, downForThresholds);

	msgbuf << "\n* Icinga 2 last seen: " << heartbeatLag << " seconds ago";
	checkGreater(heartbeatLag, heartbeatThresholds);

	msgbuf << "\n* " << (weResponsible ? "Responsible" : "Not responsible") << " for: " << responsibleFor << " seconds";

	if (otherResponsible) {
		msgbuf << " (but other instance is responsible)";
	} else {
		checkGreater(idleFor, idleForThresholds);
	}

	msgbuf << "\n* History backlog: " << historyBacklog << " seconds";
	checkGreater(historyBacklog, historyBacklogThresholds);

	msgbuf << "\n* Last sync: ";

	if (syncWhen) {
		msgbuf << syncAgo << " seconds ago";
		perfdata->Add(new PerfdataValue("sync_ago", syncAgo, false, "seconds", syncAgoThresholds.Warning, syncAgoThresholds.Critical, 0));
	} else {
		msgbuf << "never";
	}

	checkGreater(syncAgo, syncAgoThresholds);

	if (syncWhen) {
		msgbuf << "\n* Last sync took: " << syncTook << " seconds";
		checkGreater(syncTook, syncTookThresholds);
		perfdata->Add(new PerfdataValue("sync_took", syncTook, false, "seconds", syncTookThresholds.Warning, syncTookThresholds.Critical, 0));
	}

	perfdata->Add(new PerfdataValue("queries", qps, false, "", Empty, Empty, 0));
	perfdata->Add(new PerfdataValue("queries_1min", redis->GetQueryCount(60), Empty, Empty, 0));
	perfdata->Add(new PerfdataValue("queries_5mins", redis->GetQueryCount(5 * 60), Empty, Empty, 0));
	perfdata->Add(new PerfdataValue("queries_15mins", redis->GetQueryCount(15 * 60), Empty, Empty, 0));
	perfdata->Add(new PerfdataValue("pending_queries", pendingQueries, false, "", pendingQueriesThresholds.Warning, pendingQueriesThresholds.Critical, 0));
	perfdata->Add(new PerfdataValue("down_for", downFor, false, "seconds", downForThresholds.Warning, downForThresholds.Critical, 0));
	perfdata->Add(new PerfdataValue("heartbeat_lag", heartbeatLag, false, "seconds", heartbeatThresholds.Warning, heartbeatThresholds.Critical));
	perfdata->Add(new PerfdataValue("idle_for", idleFor, false, "seconds", idleForThresholds.Warning, idleForThresholds.Critical));
	perfdata->Add(new PerfdataValue("history_backlog", historyBacklog, false, "seconds", historyBacklogThresholds.Warning, historyBacklogThresholds.Critical, 0));

	for (auto& kv : statsPerOp) {
		auto perMin (kv.second.UpdateAndGetValues(now, 60));
		auto perSec (perMin / 60.0);
		auto thresholds (thresholdsByOp.find(kv.first));

		msgbuf << "\n* " << perSec << " " << kv.first << "/s";

		if (thresholds != thresholdsByOp.end()) {
			checkLower(perSec, thresholds->second);
		}

		perfdata->Add(new PerfdataValue(kv.first, perSec, false, "", Empty, Empty, 0));
		perfdata->Add(new PerfdataValue(kv.first + "_1min", perMin, false, "", Empty, Empty, 0));
		perfdata->Add(new PerfdataValue(kv.first + "_5mins", kv.second.UpdateAndGetValues(now, 5 * 60), false, "", Empty, Empty, 0));
		perfdata->Add(new PerfdataValue(kv.first + "_15mins", kv.second.UpdateAndGetValues(now, 15 * 60), false, "", Empty, Empty, 0));
	}

	{
		static boost::regex wellNamedUnits (":(bytes|seconds)$");
		ObjectLock lock (goMetricsByCumulativity);

		for (auto& kv : goMetricsByCumulativity) {
			bool cumulative = kv.first == "cumulative";
			Dictionary::Ptr goMetricsPerCumulativity = kv.second;
			ObjectLock lock (goMetricsPerCumulativity);

			for (auto& kv : goMetricsPerCumulativity) {
				std::string unit;
				boost::smatch what;

				if (boost::regex_search(kv.first.GetData(), what, wellNamedUnits)) {
					unit = what[1];
				}

				bool counter = cumulative && unit.empty();

				perfdata->Add(new PerfdataValue(kv.first, kv.second, counter, std::move(unit)));
			}
		}
	}

	cr->SetPerformanceData(perfdata);

	ReportIcingadbCheck(checkable, commandObj, cr, msgbuf.str(), state);
}
