/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2017 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#include "perfdata/graphitewriter.hpp"
#include "perfdata/graphitewriter.tcpp"
#include "icinga/service.hpp"
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
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/replace.hpp>

using namespace icinga;

REGISTER_TYPE(GraphiteWriter);

REGISTER_STATSFUNCTION(GraphiteWriter, &GraphiteWriter::StatsFunc);

GraphiteWriter::GraphiteWriter(void)
    : m_WorkQueue(10000000, 1)
{ }

void GraphiteWriter::OnConfigLoaded(void)
{
	ObjectImpl<GraphiteWriter>::OnConfigLoaded();

	m_WorkQueue.SetName("GraphiteWriter, " + GetName());
}

void GraphiteWriter::StatsFunc(const Dictionary::Ptr& status, const Array::Ptr& perfdata)
{
	Dictionary::Ptr nodes = new Dictionary();

	for (const GraphiteWriter::Ptr& graphitewriter : ConfigType::GetObjectsByType<GraphiteWriter>()) {
		size_t workQueueItems = graphitewriter->m_WorkQueue.GetLength();
		double workQueueItemRate = graphitewriter->m_WorkQueue.GetTaskCount(60) / 60.0;

		Dictionary::Ptr stats = new Dictionary();
		stats->Set("work_queue_items", workQueueItems);
		stats->Set("work_queue_item_rate", workQueueItemRate);
		stats->Set("connected", graphitewriter->GetConnected());

		nodes->Set(graphitewriter->GetName(), stats);

		perfdata->Add(new PerfdataValue("graphitewriter_" + graphitewriter->GetName() + "_work_queue_items", workQueueItems));
		perfdata->Add(new PerfdataValue("graphitewriter_" + graphitewriter->GetName() + "_work_queue_item_rate", workQueueItemRate));
	}

	status->Set("graphitewriter", nodes);
}

void GraphiteWriter::Start(bool runtimeCreated)
{
	ObjectImpl<GraphiteWriter>::Start(runtimeCreated);

	Log(LogInformation, "GraphiteWriter")
	    << "'" << GetName() << "' started.";

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

void GraphiteWriter::Stop(bool runtimeRemoved)
{
	Log(LogInformation, "GraphiteWriter")
	    << "'" << GetName() << "' stopped.";

	m_WorkQueue.Join();

	ObjectImpl<GraphiteWriter>::Stop(runtimeRemoved);
}

void GraphiteWriter::AssertOnWorkQueue(void)
{
	ASSERT(m_WorkQueue.IsWorkerThread());
}

void GraphiteWriter::ExceptionHandler(boost::exception_ptr exp)
{
	Log(LogCritical, "GraphiteWriter", "Exception during Graphite operation: Verify that your backend is operational!");

	Log(LogDebug, "GraphiteWriter")
	    << "Exception during Graphite operation: " << DiagnosticInformation(exp);

	if (GetConnected()) {
		m_Stream->Close();

		SetConnected(false);
	}
}

void GraphiteWriter::Reconnect(void)
{
	AssertOnWorkQueue();

	double startTime = Utility::GetTime();

	CONTEXT("Reconnecting to Graphite '" + GetName() + "'");

	SetShouldConnect(true);

	if (GetConnected())
		return;

	TcpSocket::Ptr socket = new TcpSocket();

	Log(LogNotice, "GraphiteWriter")
	    << "Reconnecting to Graphite on host '" << GetHost() << "' port '" << GetPort() << "'.";

	try {
		socket->Connect(GetHost(), GetPort());
	} catch (const std::exception& ex) {
		Log(LogCritical, "GraphiteWriter")
		    << "Can't connect to Graphite on host '" << GetHost() << "' port '" << GetPort() << "'.";
		throw ex;
	}

	m_Stream = new NetworkStream(socket);

	SetConnected(true);

	Log(LogInformation, "GraphiteWriter")
	    << "Finished reconnecting to Graphite in " << std::setw(2) << Utility::GetTime() - startTime << " second(s).";
}

void GraphiteWriter::ReconnectTimerHandler(void)
{
	m_WorkQueue.Enqueue(std::bind(&GraphiteWriter::Reconnect, this), PriorityNormal);
}

void GraphiteWriter::Disconnect(void)
{
	AssertOnWorkQueue();

	if (!GetConnected())
		return;

	m_Stream->Close();

	SetConnected(false);
}

void GraphiteWriter::CheckResultHandler(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	m_WorkQueue.Enqueue(std::bind(&GraphiteWriter::CheckResultHandlerInternal, this, checkable, cr));
}

void GraphiteWriter::CheckResultHandlerInternal(const Checkable::Ptr& checkable, const CheckResult::Ptr& cr)
{
	AssertOnWorkQueue();

	CONTEXT("Processing check result for '" + checkable->GetName() + "'");

	if (!IcingaApplication::GetInstance()->GetEnablePerfdata() || !checkable->GetEnablePerfdata())
		return;

	Host::Ptr host;
	Service::Ptr service;
	tie(host, service) = GetHostService(checkable);

	MacroProcessor::ResolverList resolvers;
	if (service)
		resolvers.push_back(std::make_pair("service", service));
	resolvers.push_back(std::make_pair("host", host));
	resolvers.push_back(std::make_pair("icinga", IcingaApplication::GetInstance()));

	String prefix;

	if (service) {
		prefix = MacroProcessor::ResolveMacros(GetServiceNameTemplate(), resolvers, cr, NULL, std::bind(&GraphiteWriter::EscapeMacroMetric, _1));
	} else {
		prefix = MacroProcessor::ResolveMacros(GetHostNameTemplate(), resolvers, cr, NULL, std::bind(&GraphiteWriter::EscapeMacroMetric, _1));
	}

	String prefixPerfdata = prefix + ".perfdata";
	String prefixMetadata = prefix + ".metadata";

	double ts = cr->GetExecutionEnd();

	if (GetEnableSendMetadata()) {
		if (service) {
			SendMetric(prefixMetadata, "state", service->GetState(), ts);
		} else {
			SendMetric(prefixMetadata, "state", host->GetState(), ts);
		}

		SendMetric(prefixMetadata, "current_attempt", checkable->GetCheckAttempt(), ts);
		SendMetric(prefixMetadata, "max_check_attempts", checkable->GetMaxCheckAttempts(), ts);
		SendMetric(prefixMetadata, "state_type", checkable->GetStateType(), ts);
		SendMetric(prefixMetadata, "reachable", checkable->IsReachable(), ts);
		SendMetric(prefixMetadata, "downtime_depth", checkable->GetDowntimeDepth(), ts);
		SendMetric(prefixMetadata, "acknowledgement", checkable->GetAcknowledgement(), ts);
		SendMetric(prefixMetadata, "latency", cr->CalculateLatency(), ts);
		SendMetric(prefixMetadata, "execution_time", cr->CalculateExecutionTime(), ts);
	}

	SendPerfdata(prefixPerfdata, cr, ts);
}

void GraphiteWriter::SendPerfdata(const String& prefix, const CheckResult::Ptr& cr, double ts)
{
	Array::Ptr perfdata = cr->GetPerformanceData();

	if (!perfdata)
		return;

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
				    << "Ignoring invalid perfdata value: " << val;
				continue;
			}
		}

		String escapedKey = EscapeMetricLabel(pdv->GetLabel());

		SendMetric(prefix, escapedKey + ".value", pdv->GetValue(), ts);

		if (GetEnableSendThresholds()) {
			if (pdv->GetCrit())
				SendMetric(prefix, escapedKey + ".crit", pdv->GetCrit(), ts);
			if (pdv->GetWarn())
				SendMetric(prefix, escapedKey + ".warn", pdv->GetWarn(), ts);
			if (pdv->GetMin())
				SendMetric(prefix, escapedKey + ".min", pdv->GetMin(), ts);
			if (pdv->GetMax())
				SendMetric(prefix, escapedKey + ".max", pdv->GetMax(), ts);
		}
	}
}

void GraphiteWriter::SendMetric(const String& prefix, const String& name, double value, double ts)
{
	std::ostringstream msgbuf;
	msgbuf << prefix << "." << name << " " << Convert::ToString(value) << " " << static_cast<long>(ts);

	Log(LogDebug, "GraphiteWriter")
	    << "Add to metric list:'" << msgbuf.str() << "'.";

	// do not send \n to debug log
	msgbuf << "\n";
	String metric = msgbuf.str();

	ObjectLock olock(this);

	if (!GetConnected())
		return;

	try {
		m_Stream->Write(metric.CStr(), metric.GetLength());
	} catch (const std::exception& ex) {
		Log(LogCritical, "GraphiteWriter")
		    << "Cannot write to TCP socket on host '" << GetHost() << "' port '" << GetPort() << "'.";

		throw ex;
	}
}

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

Value GraphiteWriter::EscapeMacroMetric(const Value& value)
{
	if (value.IsObjectType<Array>()) {
		Array::Ptr arr = value;
		Array::Ptr result = new Array();

		ObjectLock olock(arr);
		for (const Value& arg : arr) {
			result->Add(EscapeMetric(arg));
		}

		return Utility::Join(result, '.');
	} else
		return EscapeMetric(value);
}

void GraphiteWriter::ValidateHostNameTemplate(const String& value, const ValidationUtils& utils)
{
	ObjectImpl<GraphiteWriter>::ValidateHostNameTemplate(value, utils);

	if (!MacroProcessor::ValidateMacroString(value))
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("host_name_template"), "Closing $ not found in macro format string '" + value + "'."));
}

void GraphiteWriter::ValidateServiceNameTemplate(const String& value, const ValidationUtils& utils)
{
	ObjectImpl<GraphiteWriter>::ValidateServiceNameTemplate(value, utils);

	if (!MacroProcessor::ValidateMacroString(value))
		BOOST_THROW_EXCEPTION(ValidationError(this, boost::assign::list_of("service_name_template"), "Closing $ not found in macro format string '" + value + "'."));
}
