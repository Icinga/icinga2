/* Icinga 2 | (c) 2022 Icinga GmbH | GPLv2+ */

#include "icingadb/icingadbchecktask.hpp"
#include "icinga/host.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/pluginutility.hpp"
#include "base/function.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
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

	auto resolve ([&](const String& macro) {
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

	auto dumpTakesThresholds (resolveThresholds("$icingadb_full_dump_duration_warning$", "$icingadb_full_dump_duration_critical$"));
	auto syncTakesThresholds (resolveThresholds("$icingadb_full_sync_duration_warning$", "$icingadb_full_sync_duration_critical$"));
	auto icingaBacklogThresholds (resolveThresholds("$icingadb_redis_backlog_warning$", "$icingadb_redis_backlog_critical$"));
	auto icingadbBacklogThresholds (resolveThresholds("$icingadb_database_backlog_warning$", "$icingadb_database_backlog_critical$"));

	if (resolvedMacros && !useResolvedMacros)
		return;

	if (icingadbName.IsEmpty()) {
		ReportIcingadbCheck(checkable, commandObj, cr, "Icinga DB UNKNOWN: Attribute 'icingadb_name' must be set.", ServiceUnknown);
		return;
	}

	auto conn (IcingaDB::GetByName(icingadbName));

	if (!conn) {
		ReportIcingadbCheck(checkable, commandObj, cr, "Icinga DB UNKNOWN: Icinga DB connection '" + icingadbName + "' does not exist.", ServiceUnknown);
		return;
	}

	auto redis (conn->GetConnection());

	if (!redis || !redis->GetConnected()) {
		ReportIcingadbCheck(checkable, commandObj, cr, "Icinga DB CRITICAL: Not connected to Redis.", ServiceCritical);
		return;
	}

	auto now (Utility::GetTime());
	Array::Ptr redisTime, xReadHeartbeat, xReadStats, xReadRuntimeBacklog, xReadHistoryBacklog;

	try {
		auto replies (redis->GetResultsOfQueries(
			{
				{"TIME"},
				{"XREAD", "STREAMS", "icingadb:telemetry:heartbeat", "0-0"},
				{"XREAD", "STREAMS", "icingadb:telemetry:stats", "0-0"},
				{"XREAD", "COUNT", "1", "STREAMS", "icinga:runtime", "icinga:runtime:state", "0-0", "0-0"},
				{
					"XREAD", "COUNT", "1", "STREAMS",
					"icinga:history:stream:acknowledgement",
					"icinga:history:stream:comment",
					"icinga:history:stream:downtime",
					"icinga:history:stream:flapping",
					"icinga:history:stream:notification",
					"icinga:history:stream:state",
					"0-0", "0-0", "0-0", "0-0", "0-0", "0-0",
				}
			},
			RedisConnection::QueryPriority::Heartbeat
		));

		redisTime = std::move(replies.at(0));
		xReadHeartbeat = std::move(replies.at(1));
		xReadStats = std::move(replies.at(2));
		xReadRuntimeBacklog = std::move(replies.at(3));
		xReadHistoryBacklog = std::move(replies.at(4));
	} catch (const std::exception& ex) {
		ReportIcingadbCheck(
			checkable, commandObj, cr,
			String("Icinga DB CRITICAL: Could not query Redis: ") + ex.what(), ServiceCritical
		);
		return;
	}

	if (!xReadHeartbeat) {
		ReportIcingadbCheck(
			checkable, commandObj, cr,
			"Icinga DB CRITICAL: The Icinga DB daemon seems to have never run. (Missing heartbeat)",
			ServiceCritical
		);

		return;
	}

	auto redisOldestPending (redis->GetOldestPendingQueryTs());
	auto ongoingDumpStart (conn->GetOngoingDumpStart());
	auto dumpWhen (conn->GetLastdumpEnd());
	auto dumpTook (conn->GetLastdumpTook());

	auto redisNow (Convert::ToLong(redisTime->Get(0)) + Convert::ToLong(redisTime->Get(1)) / 1000000.0);
	Array::Ptr heartbeatMessage = Array::Ptr(Array::Ptr(xReadHeartbeat->Get(0))->Get(1))->Get(0);
	auto heartbeatTime (GetXMessageTs(heartbeatMessage));
	std::map<String, String> heartbeatData;

	IcingaDB::AddKvsToMap(heartbeatMessage->Get(1), heartbeatData);

	String version = heartbeatData.at("version");
	auto icingadbNow (Convert::ToLong(heartbeatData.at("time")) / 1000.0 + (redisNow - heartbeatTime));
	auto icingadbStartTime (Convert::ToLong(heartbeatData.at("start-time")) / 1000.0);
	String errMsg (heartbeatData.at("error"));
	auto errSince (Convert::ToLong(heartbeatData.at("error-since")) / 1000.0);
	String perfdataFromRedis = heartbeatData.at("performance-data");
	auto heartbeatLastReceived (Convert::ToLong(heartbeatData.at("last-heartbeat-received")) / 1000.0);
	bool weResponsible = Convert::ToLong(heartbeatData.at("ha-responsible"));
	auto weResponsibleTs (Convert::ToLong(heartbeatData.at("ha-responsible-ts")) / 1000.0);
	bool otherResponsible = Convert::ToLong(heartbeatData.at("ha-other-responsible"));
	auto syncOngoingSince (Convert::ToLong(heartbeatData.at("sync-ongoing-since")) / 1000.0);
	auto syncSuccessWhen (Convert::ToLong(heartbeatData.at("sync-success-finish")) / 1000.0);
	auto syncSuccessTook (Convert::ToLong(heartbeatData.at("sync-success-duration")) / 1000.0);

	std::ostringstream i2okmsgs, idbokmsgs, warnmsgs, critmsgs;
	Array::Ptr perfdata = new Array();

	i2okmsgs << std::fixed << std::setprecision(3);
	idbokmsgs << std::fixed << std::setprecision(3);
	warnmsgs << std::fixed << std::setprecision(3);
	critmsgs << std::fixed << std::setprecision(3);

	const auto downForCritical (10);
	auto downFor (redisNow - heartbeatTime);
	bool down = false;

	if (downFor > downForCritical) {
		down = true;

		critmsgs << " Last seen " << Utility::FormatDuration(downFor)
			<< " ago, greater than CRITICAL threshold (" << Utility::FormatDuration(downForCritical) << ")!";
	} else {
		idbokmsgs << "\n* Last seen: " << Utility::FormatDuration(downFor) << " ago";
	}

	perfdata->Add(new PerfdataValue("icingadb_heartbeat_age", downFor, false, "seconds", Empty, downForCritical, 0));

	const auto errForCritical (10);
	auto err (!errMsg.IsEmpty());
	auto errFor (icingadbNow - errSince);

	if (err) {
		if (errFor > errForCritical) {
			critmsgs << " ERROR: " << errMsg << "!";
		}

		perfdata->Add(new PerfdataValue("error_for", errFor * (err ? 1 : -1), false, "seconds", Empty, errForCritical, 0));
	}

	if (!down) {
		const auto heartbeatLagWarning (3/* Icinga DB read freq. */ + 1/* Icinga DB write freq. */ + 2/* threshold */);
		auto heartbeatLag (fmin(icingadbNow - heartbeatLastReceived, 10 * 60));

		if (!heartbeatLastReceived) {
			critmsgs << " Lost Icinga 2 heartbeat!";
		} else if (heartbeatLag > heartbeatLagWarning) {
			warnmsgs << " Icinga 2 heartbeat lag: " << Utility::FormatDuration(heartbeatLag)
				<< ", greater than WARNING threshold (" << Utility::FormatDuration(heartbeatLagWarning) << ").";
		}

		perfdata->Add(new PerfdataValue("icinga2_heartbeat_age", heartbeatLag, false, "seconds", heartbeatLagWarning, Empty, 0));
	}

	if (weResponsible) {
		idbokmsgs << "\n* Responsible";
	} else if (otherResponsible) {
		idbokmsgs << "\n* Not responsible, but another instance is";
	} else {
		critmsgs << " No instance is responsible!";
	}

	perfdata->Add(new PerfdataValue("icingadb_responsible_instances", int(weResponsible || otherResponsible), false, "", Empty, Empty, 0, 1));

	const auto clockDriftWarning (5);
	const auto clockDriftCritical (30);
	auto clockDrift (std::max({
		fabs(now - redisNow),
		fabs(redisNow - icingadbNow),
		fabs(icingadbNow - now),
	}));

	if (clockDrift > clockDriftCritical) {
		critmsgs << " Icinga 2/Redis/Icinga DB clock drift: " << Utility::FormatDuration(clockDrift)
			<< ", greater than CRITICAL threshold (" << Utility::FormatDuration(clockDriftCritical) << ")!";
	} else if (clockDrift > clockDriftWarning) {
		warnmsgs << " Icinga 2/Redis/Icinga DB clock drift: " << Utility::FormatDuration(clockDrift)
			<< ", greater than WARNING threshold (" << Utility::FormatDuration(clockDriftWarning) << ").";
	}

	perfdata->Add(new PerfdataValue("clock_drift", clockDrift, false, "seconds", clockDriftWarning, clockDriftCritical, 0));

	if (ongoingDumpStart) {
		auto ongoingDumpTakes (now - ongoingDumpStart);

		if (!dumpTakesThresholds.Critical.IsEmpty() && ongoingDumpTakes > dumpTakesThresholds.Critical) {
			critmsgs << " Current Icinga 2 full dump already takes " << Utility::FormatDuration(ongoingDumpTakes)
				<< ", greater than CRITICAL threshold (" << Utility::FormatDuration(dumpTakesThresholds.Critical) << ")!";
		} else if (!dumpTakesThresholds.Warning.IsEmpty() && ongoingDumpTakes > dumpTakesThresholds.Warning) {
			warnmsgs << " Current Icinga 2 full dump already takes " << Utility::FormatDuration(ongoingDumpTakes)
				<< ", greater than WARNING threshold (" << Utility::FormatDuration(dumpTakesThresholds.Warning) << ").";
		} else {
			i2okmsgs << "\n* Current full dump running for " << Utility::FormatDuration(ongoingDumpTakes);
		}

		perfdata->Add(new PerfdataValue("icinga2_current_full_dump_duration", ongoingDumpTakes, false, "seconds",
			dumpTakesThresholds.Warning, dumpTakesThresholds.Critical, 0));
	}

	if (!down && syncOngoingSince) {
		auto ongoingSyncTakes (icingadbNow - syncOngoingSince);

		if (!syncTakesThresholds.Critical.IsEmpty() && ongoingSyncTakes > syncTakesThresholds.Critical) {
			critmsgs << " Current full sync already takes " << Utility::FormatDuration(ongoingSyncTakes)
				<< ", greater than CRITICAL threshold (" << Utility::FormatDuration(syncTakesThresholds.Critical) << ")!";
		} else if (!syncTakesThresholds.Warning.IsEmpty() && ongoingSyncTakes > syncTakesThresholds.Warning) {
			warnmsgs << " Current full sync already takes " << Utility::FormatDuration(ongoingSyncTakes)
				<< ", greater than WARNING threshold (" << Utility::FormatDuration(syncTakesThresholds.Warning) << ").";
		} else {
			idbokmsgs << "\n* Current full sync running for " << Utility::FormatDuration(ongoingSyncTakes);
		}

		perfdata->Add(new PerfdataValue("icingadb_current_full_sync_duration", ongoingSyncTakes, false, "seconds",
			syncTakesThresholds.Warning, syncTakesThresholds.Critical, 0));
	}

	auto redisBacklog (now - redisOldestPending);

	if (!redisOldestPending) {
		redisBacklog = 0;
	}

	if (!icingaBacklogThresholds.Critical.IsEmpty() && redisBacklog > icingaBacklogThresholds.Critical) {
		critmsgs << " Icinga 2 Redis query backlog: " << Utility::FormatDuration(redisBacklog)
			<< ", greater than CRITICAL threshold (" << Utility::FormatDuration(icingaBacklogThresholds.Critical) << ")!";
	} else if (!icingaBacklogThresholds.Warning.IsEmpty() && redisBacklog > icingaBacklogThresholds.Warning) {
		warnmsgs << " Icinga 2 Redis query backlog: " << Utility::FormatDuration(redisBacklog)
			<< ", greater than WARNING threshold (" << Utility::FormatDuration(icingaBacklogThresholds.Warning) << ").";
	}

	perfdata->Add(new PerfdataValue("icinga2_redis_query_backlog", redisBacklog, false, "seconds",
		icingaBacklogThresholds.Warning, icingaBacklogThresholds.Critical, 0));

	if (!down) {
		auto getBacklog = [redisNow](const Array::Ptr& streams) -> double {
			if (!streams) {
				return 0;
			}

			double minTs = 0;
			ObjectLock lock (streams);

			for (Array::Ptr stream : streams) {
				auto ts (GetXMessageTs(Array::Ptr(stream->Get(1))->Get(0)));

				if (minTs == 0 || ts < minTs) {
					minTs = ts;
				}
			}

			if (minTs > 0) {
				return redisNow - minTs;
			} else {
				return 0;
			}
		};

		double historyBacklog = getBacklog(xReadHistoryBacklog);

		if (!icingadbBacklogThresholds.Critical.IsEmpty() && historyBacklog > icingadbBacklogThresholds.Critical) {
			critmsgs << " History backlog: " << Utility::FormatDuration(historyBacklog)
				<< ", greater than CRITICAL threshold (" << Utility::FormatDuration(icingadbBacklogThresholds.Critical) << ")!";
		} else if (!icingadbBacklogThresholds.Warning.IsEmpty() && historyBacklog > icingadbBacklogThresholds.Warning) {
			warnmsgs << " History backlog: " << Utility::FormatDuration(historyBacklog)
				<< ", greater than WARNING threshold (" << Utility::FormatDuration(icingadbBacklogThresholds.Warning) << ").";
		}

		perfdata->Add(new PerfdataValue("icingadb_history_backlog", historyBacklog, false, "seconds",
			icingadbBacklogThresholds.Warning, icingadbBacklogThresholds.Critical, 0));

		double runtimeBacklog = 0;

		if (weResponsible && !syncOngoingSince) {
			// These streams are only processed by the responsible instance after the full sync finished,
			// it's fine for some backlog to exist otherwise.
			runtimeBacklog = getBacklog(xReadRuntimeBacklog);

			if (!icingadbBacklogThresholds.Critical.IsEmpty() && runtimeBacklog > icingadbBacklogThresholds.Critical) {
				critmsgs << " Runtime update backlog: " << Utility::FormatDuration(runtimeBacklog)
					<< ", greater than CRITICAL threshold (" << Utility::FormatDuration(icingadbBacklogThresholds.Critical) << ")!";
			} else if (!icingadbBacklogThresholds.Warning.IsEmpty() && runtimeBacklog > icingadbBacklogThresholds.Warning) {
				warnmsgs << " Runtime update backlog: " << Utility::FormatDuration(runtimeBacklog)
					<< ", greater than WARNING threshold (" << Utility::FormatDuration(icingadbBacklogThresholds.Warning) << ").";
			}
		}

		// Also report the perfdata value on the standby instance or during a full sync (as 0 in this case).
		perfdata->Add(new PerfdataValue("icingadb_runtime_update_backlog", runtimeBacklog, false, "seconds",
			icingadbBacklogThresholds.Warning, icingadbBacklogThresholds.Critical, 0));
	}

	auto dumpAgo (now - dumpWhen);

	if (dumpWhen) {
		perfdata->Add(new PerfdataValue("icinga2_last_full_dump_ago", dumpAgo, false, "seconds", Empty, Empty, 0));
	}

	if (dumpTook) {
		perfdata->Add(new PerfdataValue("icinga2_last_full_dump_duration", dumpTook, false, "seconds", Empty, Empty, 0));
	}

	if (dumpWhen && dumpTook) {
		i2okmsgs << "\n* Last full dump: " << Utility::FormatDuration(dumpAgo)
			<< " ago, took " << Utility::FormatDuration(dumpTook);
	}

	auto icingadbUptime (icingadbNow - icingadbStartTime);

	if (!down) {
		perfdata->Add(new PerfdataValue("icingadb_uptime", icingadbUptime, false, "seconds", Empty, Empty, 0));
	}

	{
		Array::Ptr values = PluginUtility::SplitPerfdata(perfdataFromRedis);
		ObjectLock lock (values);

		for (auto& v : values) {
			perfdata->Add(PerfdataValue::Parse(v));
		}
	}

	if (weResponsibleTs) {
		perfdata->Add(new PerfdataValue("icingadb_responsible_for",
			(weResponsible ? 1 : -1) * (icingadbNow - weResponsibleTs), false, "seconds"));
	}

	auto syncAgo (icingadbNow - syncSuccessWhen);

	if (syncSuccessWhen) {
		perfdata->Add(new PerfdataValue("icingadb_last_full_sync_ago", syncAgo, false, "seconds", Empty, Empty, 0));
	}

	if (syncSuccessTook) {
		perfdata->Add(new PerfdataValue("icingadb_last_full_sync_duration", syncSuccessTook, false, "seconds", Empty, Empty, 0));
	}

	if (syncSuccessWhen && syncSuccessTook) {
		idbokmsgs << "\n* Last full sync: " << Utility::FormatDuration(syncAgo)
			<< " ago, took " << Utility::FormatDuration(syncSuccessTook);
	}

	std::map<String, RingBuffer> statsPerOp;

	const char * const icingadbKnownStats[] = {
		"config_sync", "state_sync", "history_sync", "overdue_sync", "history_cleanup"
	};

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

	for (auto& kv : statsPerOp) {
		perfdata->Add(new PerfdataValue("icingadb_" + kv.first + "_items_1min", kv.second.UpdateAndGetValues(now, 60), false, "", Empty, Empty, 0));
		perfdata->Add(new PerfdataValue("icingadb_" + kv.first + "_items_5mins", kv.second.UpdateAndGetValues(now, 5 * 60), false, "", Empty, Empty, 0));
		perfdata->Add(new PerfdataValue("icingadb_" + kv.first + "_items_15mins", kv.second.UpdateAndGetValues(now, 15 * 60), false, "", Empty, Empty, 0));
	}

	perfdata->Add(new PerfdataValue("icinga2_redis_queries_1min", redis->GetQueryCount(60), false, "", Empty, Empty, 0));
	perfdata->Add(new PerfdataValue("icinga2_redis_queries_5mins", redis->GetQueryCount(5 * 60), false, "", Empty, Empty, 0));
	perfdata->Add(new PerfdataValue("icinga2_redis_queries_15mins", redis->GetQueryCount(15 * 60), false, "", Empty, Empty, 0));

	perfdata->Add(new PerfdataValue("icinga2_redis_pending_queries", redis->GetPendingQueryCount(), false, "", Empty, Empty, 0));

	struct {
		const char * Name;
		int (RedisConnection::* Getter)(RingBuffer::SizeType span, RingBuffer::SizeType tv);
	} const icingaWriteSubjects[] = {
		{"config_dump", &RedisConnection::GetWrittenConfigFor},
		{"state_dump", &RedisConnection::GetWrittenStateFor},
		{"history_dump", &RedisConnection::GetWrittenHistoryFor}
	};

	for (auto subject : icingaWriteSubjects) {
		perfdata->Add(new PerfdataValue(String("icinga2_") + subject.Name + "_items_1min", (redis.get()->*subject.Getter)(60, now), false, "", Empty, Empty, 0));
		perfdata->Add(new PerfdataValue(String("icinga2_") + subject.Name + "_items_5mins", (redis.get()->*subject.Getter)(5 * 60, now), false, "", Empty, Empty, 0));
		perfdata->Add(new PerfdataValue(String("icinga2_") + subject.Name + "_items_15mins", (redis.get()->*subject.Getter)(15 * 60, now), false, "", Empty, Empty, 0));
	}

	ServiceState state;
	std::ostringstream msgbuf;
	auto i2okmsg (i2okmsgs.str());
	auto idbokmsg (idbokmsgs.str());
	auto warnmsg (warnmsgs.str());
	auto critmsg (critmsgs.str());

	msgbuf << "Icinga DB ";

	if (!critmsg.empty()) {
		state = ServiceCritical;
		msgbuf << "CRITICAL:" << critmsg;

		if (!warnmsg.empty()) {
			msgbuf << "\n\nWARNING:" << warnmsg;
		}
	} else if (!warnmsg.empty()) {
		state = ServiceWarning;
		msgbuf << "WARNING:" << warnmsg;
	} else {
		state = ServiceOK;
		msgbuf << "OK: Uptime: " << Utility::FormatDuration(icingadbUptime) << ". Version: " << version << ".";
	}

	if (!i2okmsg.empty()) {
		msgbuf << "\n\nIcinga 2:\n" << i2okmsg;
	}

	if (!idbokmsg.empty()) {
		msgbuf << "\n\nIcinga DB:\n" << idbokmsg;
	}

	cr->SetPerformanceData(perfdata);
	ReportIcingadbCheck(checkable, commandObj, cr, msgbuf.str(), state);
}
