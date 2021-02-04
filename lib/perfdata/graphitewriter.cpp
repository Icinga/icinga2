/* Icinga 2 | (c) 2012 Icinga GmbH | GPLv2+ */

#include "perfdata/graphitewriter.hpp"
#include "perfdata/graphitewriter-ti.cpp"
#include "icinga/service.hpp"
#include "icinga/checkcommand.hpp"
#include "icinga/macroprocessor.hpp"
#include "icinga/icingaapplication.hpp"
#include "base/tcpsocket.hpp"
#include "base/configtype.hpp"
#include "base/objectlock.hpp"
#include "base/logger.hpp"
#include "base/convert.hpp"
#include "base/utility.hpp"
#include "base/perfdatavalue.hpp"
#include "base/application.hpp"
#include "base/stream.hpp"
#include "base/networkstream.hpp"
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
			{ "connected", graphitewriter->GetConnected() }
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
	m_WorkQueue.SetExceptionCallback(std::bind(&GraphiteWriter::ExceptionHandler, this, _1));

	/* Timer for reconnecting */
	m_ReconnectTimer = new Timer();
	m_ReconnectTimer->SetInterval(10);
	m_ReconnectTimer->OnTimerExpired.connect(std::bind(&GraphiteWriter::ReconnectTimerHandler, this));
	m_ReconnectTimer->Start();
	m_ReconnectTimer->Reschedule(0);

	/* Register event handlers. */
	Checkable::OnNewCheckResult.connect(std::bind(&GraphiteWriter::CheckResultHandler, this, _1, _2));
}

/**
 * Pause is equivalent to Stop, but with HA capabilities to resume at runtime.
 */
void GraphiteWriter::Pause()
{
	m_ReconnectTimer.reset();

	try {
		ReconnectInternal();
	} catch (const std::exception&) {
		Log(LogInformation, "GraphiteWriter")
			<< "'" << GetName() << "' paused. Unable to connect, not flushing buffers. Data may be lost on reload.";

		ObjectImpl<GraphiteWriter>::Pause();
		return;
	}

	m_WorkQueue.Join();
	DisconnectInternal();

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

	if (GetConnected()) {
		m_Stream->close();

		SetConnected(false);
	}
}

/**
 * Reconnect method, stops when the feature is paused in HA zones.
 *
 * Called inside the WQ.
 */
void GraphiteWriter::Reconnect()
{
	AssertOnWorkQueue();

	if (IsPaused()) {
		SetConnected(false);
		return;
	}

	ReconnectInternal();
}

/**
 * Reconnect method, connects to a TCP Stream
 */
void GraphiteWriter::ReconnectInternal()
{
	double startTime = Utility::GetTime();

	CONTEXT("Reconnecting to Graphite '" + GetName() + "'");

	SetShouldConnect(true);

	if (GetConnected())
		return;

	Log(LogNotice, "GraphiteWriter")
		<< "Reconnecting to Graphite on host '" << GetHost() << "' port '" << GetPort() << "'.";

	m_Stream = Shared<AsioTcpStream>::Make(IoEngine::Get().GetIoContext());

	try {
		icinga::Connect(m_Stream->lowest_layer(), GetHost(), GetPort());
	} catch (const std::exception& ex) {
		Log(LogWarning, "GraphiteWriter")
			<< "Can't connect to Graphite on host '" << GetHost() << "' port '" << GetPort() << ".'";

		SetConnected(false);

		throw;
	}

	SetConnected(true);

	Log(LogInformation, "GraphiteWriter")
		<< "Finished reconnecting to Graphite in " << std::setw(2) << Utility::GetTime() - startTime << " second(s).";
}

/**
 * Reconnect handler called by the timer.
 *
 * Enqueues a reconnect task into the WQ.
 */
void GraphiteWriter::ReconnectTimerHandler()
{
	if (IsPaused())
		return;

	m_WorkQueue.Enqueue(std::bind(&GraphiteWriter::Reconnect, this), PriorityHigh);
}

/**
 * Disconnect the stream.
 *
 * Called inside the WQ.
 */
void GraphiteWriter::Disconnect()
{
	AssertOnWorkQueue();

	DisconnectInternal();
}

/**
 * Disconnect the stream.
 *
 * Called outside the WQ.
 */
void GraphiteWriter::DisconnectInternal()
{
	if (!GetConnected())
		return;

	m_Stream->close();

	SetConnected(false);
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

	m_WorkQueue.Enqueue(std::bind(&GraphiteWriter::CheckResultHandlerInternal, this, checkable, cr));
}

/**
 * Check result event handler, prepares metadata and perfdata values and calls Send*()
 *
 * Called inside the WQ.
 *
 * @param checkable Host/Service object
 * @param cr Check result including performance data
 */
void GraphiteWriter::CheckResultHandlerInternal(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	AssertOnWorkQueue();

	CONTEXT("Processing check result for '" + checkable->GetName() + "'");

	/* TODO: Deal with missing connection here. Needs refactoring
	 * into parsing the actual performance data and then putting it
	 * into a queue for re-inserting. */

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.emplace_back("service", service);
	resolvers.emplace_back("host", host);
	resolvers.emplace_back("icinga", IcingaApplication::GetInstance());

	String prefix;

	if (service) {
		prefix = MacroProcessor::ResolveMacros(GetServiceNameTemplate(), resolvers, cr, nullptr, std::bind(&GraphiteWriter::EscapeMacroMetric, _1));
	} else {
		prefix = MacroProcessor::ResolveMacros(GetHostNameTemplate(), resolvers, cr, nullptr, std::bind(&GraphiteWriter::EscapeMacroMetric, _1));
	}

	String prefixPerfdata = prefix + ".perfdata";
	String prefixMetadata = prefix + ".metadata";

	double ts = cr->GetExecutionEnd();

	if (GetEnableSendMetadata()) {
		if (service) {
			SendMetric(checkable, prefixMetadata, "state", service->GetState(), ts);
		} else {
			SendMetric(checkable, prefixMetadata, "state", host->GetState(), ts);
		}

		SendMetric(checkable, prefixMetadata, "current_attempt", checkable->GetCheckAttempt(), ts);
		SendMetric(checkable, prefixMetadata, "max_check_attempts", checkable->GetMaxCheckAttempts(), ts);
		SendMetric(checkable, prefixMetadata, "state_type", checkable->GetStateType(), ts);
		SendMetric(checkable, prefixMetadata, "reachable", checkable->IsReachable(), ts);
		SendMetric(checkable, prefixMetadata, "downtime_depth", checkable->GetDowntimeDepth(), ts);
		SendMetric(checkable, prefixMetadata, "acknowledgement", checkable->GetAcknowledgement(), ts);
		SendMetric(checkable, prefixMetadata, "latency", cr->CalculateLatency(), ts);
		SendMetric(checkable, prefixMetadata, "execution_time", cr->CalculateExecutionTime(), ts);
	}

	SendPerfdata(checkable, prefixPerfdata, cr, ts);
}

/**
 * Parse performance data from check result and call SendMetric()
 *
 * @param checkable Host/service object
 * @param prefix Metric prefix string
 * @param cr Check result including performance data
 * @param ts Timestamp when the check result was created
 */
void GraphiteWriter::SendPerfdata(const Checkable::Ptr& checkable, const String& prefix, const CheckResult::Ptr& cr, double ts)
{
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
	namespace asio = boost::asio;

	std::ostringstream msgbuf;
	msgbuf << prefix << "." << name << " " << Convert::ToString(value) << " " << static_cast<long>(ts);

	Log(LogDebug, "GraphiteWriter")
		<< "Checkable '" << checkable->GetName() << "' adds to metric list: '" << msgbuf.str() << "'.";

	// do not send \n to debug log
	msgbuf << "\n";

	std::unique_lock<std::mutex> lock(m_StreamMutex);

	if (!GetConnected())
		return;

	try {
		asio::write(*m_Stream, asio::buffer(msgbuf.str()));
		m_Stream->flush();
	} catch (const std::exception& ex) {
		Log(LogCritical, "GraphiteWriter")
			<< "Cannot write to TCP socket on host '" << GetHost() << "' port '" << GetPort() << "'.";

		throw ex;
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
