// SPDX-FileCopyrightText: 2012 Icinga GmbH <https://icinga.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "perfdata/graphitewriter.hpp"
#include "perfdata/graphitewriter-ti.cpp"
#include "icinga/service.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/stream.hpp"
#include "base/exception.hpp"
#include "base/statsfunction.hpp"
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <utility>

using namespace icinga;

REGISTER_TYPE(GraphiteWriter);

REGISTER_STATSFUNCTION(GraphiteWriter, &GraphiteWriter::StatsFunc);

/*
 * Enable HA capabilities once the config object is loaded.
 */
void GraphiteWriter::OnConfigLoaded()
{
	ObjectImpl<GraphiteWriter>::OnConfigLoaded();

	m_WorkQueue.SetName("GraphiteWriter, " + GetName());

	if (!GetEnableHa()) {
		Log(LogDebug, "GraphiteWriter")
			<< "HA functionality disabled. Won't pause connection: " << GetName();

		SetHAMode(HARunEverywhere);
	} else {
		SetHAMode(HARunOnce);
	}
}

/**
 * Feature stats interface
 *
 * @param status Key value pairs for feature stats
 * @param perfdata Array of PerfdataValue objects
 */
void GraphiteWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	DictionaryData nodes;

	for (const GraphiteWriter::Ptr& graphitewriter : ConfigType::GetObjectsByType<GraphiteWriter>()) {
		size_t workQueueItems = graphitewriter->m_WorkQueue.GetLength();
		double workQueueItemRate = graphitewriter->m_WorkQueue.GetTaskCount(60) / 60.0;

		nodes.emplace_back(graphitewriter->GetName(), new Dictionary({
			{ "work_queue_items", workQueueItems },
			{ "work_queue_item_rate", workQueueItemRate },
			{ "connected", graphitewriter->m_Connection->IsConnected() }
		}));

		perfdata->Add(new PerfdataValue("graphitewriter_" + graphitewriter->GetName() + "_work_queue_items", workQueueItems));
		perfdata->Add(new PerfdataValue("graphitewriter_" + graphitewriter->GetName() + "_work_queue_item_rate", workQueueItemRate));
	}

	status->Set("graphitewriter", new Dictionary(std::move(nodes)));
}

/**
 * Resume is equivalent to Start, but with HA capabilities to resume at runtime.
 */
void GraphiteWriter::Resume()
{
	ObjectImpl<GraphiteWriter>::Resume();

	Log(LogInformation, "GraphiteWriter")
		<< "'" << GetName() << "' resumed.";

	/* Register exception handler for WQ tasks. */
	m_WorkQueue.SetExceptionCallback([this](boost::exception_ptr exp) { ExceptionHandler(std::move(exp)); });

	m_Connection = new PerfdataWriterConnection{"GraphiteWriter", GetName(), GetHost(), GetPort()};

	/* Register event handlers. */
	m_HandleCheckResults = Checkable::OnNewCheckResult.connect([this](const Checkable::Ptr& checkable,
		const CheckResult::Ptr& cr, const MessageOrigin::Ptr&) {
		CheckResultHandler(checkable, cr);
	});
}

/**
 * Pause is equivalent to Stop, but with HA capabilities to resume at runtime.
 */
void GraphiteWriter::Pause()
{
	m_HandleCheckResults.disconnect();

	std::promise<void> queueDonePromise;

	m_WorkQueue.Enqueue([&]() {
		queueDonePromise.set_value();
	}, PriorityLow);

	auto timeout = std::chrono::duration<double>{GetDisconnectTimeout()};
	m_Connection->CancelAfterTimeout(queueDonePromise.get_future(), timeout);

	m_WorkQueue.Join();

	Log(LogInformation, "GraphiteWriter")
		<< "'" << GetName() << "' paused.";

	ObjectImpl<GraphiteWriter>::Pause();
}

/**
 * Check if method is called inside the WQ thread.
 */
void GraphiteWriter::AssertOnWorkQueue()
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}

/**
 * Exception handler for the WQ.
 *
 * Closes the connection if connected.
 *
 * @param exp Exception pointer
 */
void GraphiteWriter::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "GraphiteWriter", "Exception during Graphite operation: Verify that your backend is operational!");

	Log(LogDebug, "GraphiteWriter")
		<< "Exception during Graphite operation: " << DiagnosticInformation(std::move(exp));
}

/**
 * Check result event handler, checks whether feature is not paused in HA setups.
 *
 * @param checkable Host/Service object
 * @param cr Check result including performance data
 */
void GraphiteWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	if (IsPaused())
		return;

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);

	String prefix;

	if (service) {
		prefix = MacroProcessor::ResolveMacros(GetServiceNameTemplate(), resolvers, cr, nullptr, [](const Value& value) -> Value {
			return EscapeMacroMetric(value);
		});
	} else {
		prefix = MacroProcessor::ResolveMacros(GetHostNameTemplate(), resolvers, cr, nullptr, [](const Value& value) -> Value {
			return EscapeMacroMetric(value);
		});
	}

	std::vector<std::pair<String, double>> metadata;
	if (GetEnableSendMetadata()) {
		metadata = {
			{"state", service ? static_cast<unsigned int>(service->GetState()) : static_cast<unsigned int>(host->GetState())},
			{"current_attempt", checkable->GetCheckAttempt()},
			{"max_check_attempts", checkable->GetMaxCheckAttempts()},
			{"state_type", checkable->GetStateType()},
			{"reachable", checkable->IsReachable()},
			{"downtime_depth", checkable->GetDowntimeDepth()},
			{"acknowledgement", checkable->GetAcknowledgement()},
			{"latency", cr->CalculateLatency()},
			{"execution_time", cr->CalculateExecutionTime()}
		};
	}

	m_WorkQueue.Enqueue([this, checkable, cr, prefix = std::move(prefix), metadata = std::move(metadata)]() {
		if (m_Connection->IsStopped()) {
			return;
		}

		CONTEXT("Processing check result for '" << checkable->GetName() << "'");

		for (auto& [name, val] : metadata) {
			SendMetric(checkable, prefix + ".metadata", name, val, cr->GetExecutionEnd());
		}

		SendPerfdata(checkable, prefix + ".perfdata", cr);
	});
}

/**
 * Parse performance data from check result and call SendMetric()
 *
 * @param checkable Host/service object
 * @param prefix Metric prefix string
 * @param cr Check result including performance data
 */
void GraphiteWriter::SendPerfdata(const Checkable::Ptr& checkable, const String& prefix, const CheckResult::Ptr& cr)
{
	AssertOnWorkQueue();

	Array::Ptr perfdata = cr->GetPerformanceData();

	if (!perfdata)
		return;

	CheckCommand::Ptr checkCommand = checkable->GetCheckCommand();

	ObjectLock olock(perfdata);
	for (const Value& val : perfdata) {
		PerfdataValue::Ptr pdv;

		if (val.IsObjectType<PerfdataValue>())
			pdv = val;
		else {
			try {
				pdv = PerfdataValue::Parse(val);
			} catch (const std::exception&) {
				Log(LogWarning, "GraphiteWriter")
					<< "Ignoring invalid perfdata for checkable '"
					<< checkable->GetName() << "' and command '"
					<< checkCommand->GetName() << "' with value: " << val;
				continue;
			}
		}

		String escapedKey = EscapeMetricLabel(pdv->GetLabel());
		double ts = cr->GetExecutionEnd();

		SendMetric(checkable, prefix, escapedKey + ".value", pdv->GetValue(), ts);

		if (GetEnableSendThresholds()) {
			if (!pdv->GetCrit().IsEmpty())
				SendMetric(checkable, prefix, escapedKey + ".crit", pdv->GetCrit(), ts);
			if (!pdv->GetWarn().IsEmpty())
				SendMetric(checkable, prefix, escapedKey + ".warn", pdv->GetWarn(), ts);
			if (!pdv->GetMin().IsEmpty())
				SendMetric(checkable, prefix, escapedKey + ".min", pdv->GetMin(), ts);
			if (!pdv->GetMax().IsEmpty())
				SendMetric(checkable, prefix, escapedKey + ".max", pdv->GetMax(), ts);
		}
	}
}

/**
 * Computes metric data and sends to Graphite
 *
 * @param checkable Host/service object
 * @param prefix Computed metric prefix string
 * @param name Metric name
 * @param value Metric value
 * @param ts Timestamp when the check result was created
 */
void GraphiteWriter::SendMetric(const Checkable::Ptr& checkable, const String& prefix, const String& name, double value, double ts)
{
	AssertOnWorkQueue();

	namespace asio = boost::asio;

	std::ostringstream msgbuf;
	msgbuf << prefix << "." << name << " " << Convert::ToString(value) << " " << static_cast<long>(ts);

	Log(LogDebug, "GraphiteWriter")
		<< "Checkable '" << checkable->GetName() << "' adds to metric list: '" << msgbuf.str() << "'.";

	// do not send \n to debug log
	msgbuf << "\n";

	try {
		m_Connection->Send(asio::buffer(msgbuf.str()));
	} catch (const PerfdataWriterConnection::Stopped& ex) {
		Log(LogDebug, "GraphiteWriter") << ex.what();
		return;
	}
}

/**
 * Escape metric tree elements
 *
 * Dots are not allowed, e.g. in host names
 *
 * @param str Metric part name
 * @return Escape string
 */
String GraphiteWriter::EscapeMetric(const String& str)
{
	String result = str;

	//don't allow '.' in metric prefixes
	boost::replace_all(result, " ", "_");
	boost::replace_all(result, ".", "_");
	boost::replace_all(result, "\\", "_");
	boost::replace_all(result, "/", "_");

	return result;
}

/**
 * Escape metric label
 *
 * Dots are allowed - users can create trees from perfdata labels
 *
 * @param str Metric label name
 * @return Escaped string
 */
String GraphiteWriter::EscapeMetricLabel(const String& str)
{
	String result = str;

	//allow to pass '.' in perfdata labels
	boost::replace_all(result, " ", "_");
	boost::replace_all(result, "\\", "_");
	boost::replace_all(result, "/", "_");
	boost::replace_all(result, "::", ".");

	return result;
}

/**
 * Escape macro metrics found via host/service name templates
 *
 * @param value Array or string with macro metric names
 * @return Escaped string. Arrays are joined with dots.
 */
Value GraphiteWriter::EscapeMacroMetric(const Value& value)
{
	if (value.IsObjectType<Array>()) {
		Array::Ptr arr = value;
		ArrayData result;

		ObjectLock olock(arr);
		for (const Value& arg : arr) {
			result.push_back(EscapeMetric(arg));
		}

		return Utility::Join(new Array(std::move(result)), '.');
	} else
		return EscapeMetric(value);
}

/**
 * Validate the configuration setting 'host_name_template'
 *
 * @param lvalue String containing runtime macros.
 * @param utils Helper, unused
 */
void GraphiteWriter::ValidateHostNameTemplate(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<GraphiteWriter>::ValidateHostNameTemplate(lvalue, utils);

	if (!MacroProcessor::ValidateMacroString(lvalue()))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "host_name_template" }, "Closing $ not found in macro format string '" + lvalue() + "'."));
}

/**
 * Validate the configuration setting 'service_name_template'
 *
 * @param lvalue String containing runtime macros.
 * @param utils Helper, unused
 */
void GraphiteWriter::ValidateServiceNameTemplate(const Lazy<String>& lvalue, const ValidationUtils& utils)
{
	ObjectImpl<GraphiteWriter>::ValidateServiceNameTemplate(lvalue, utils);

	if (!MacroProcessor::ValidateMacroString(lvalue()))
		BOOST_THROW_EXCEPTION(ValidationError(this, { "service_name_template" }, "Closing $ not found in macro format string '" + lvalue() + "'."));
}
